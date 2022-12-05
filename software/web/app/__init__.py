from flask import Flask, request, abort, render_template
import json
import os
import pytz

from . import models
from .models import db
from .memory import System7Memory

# Create app.
def create_app(extra_config=None):
    instance_path = os.environ.get("INSTANCE_PATH", "/data")

    app = Flask(__name__, instance_path=instance_path)
    app.config.from_mapping(
        SQLALCHEMY_TRACK_MODIFICATIONS=False,
        SQLALCHEMY_DATABASE_URI="sqlite:///"
        + os.path.join(app.instance_path, "db.sqlite"),
        SQLALCHEMY_ECHO=app.config["DEBUG"],
    )

    db.init_app(app)

    @app.template_filter()
    def commaify(value):
        return "{:,}".format(value)

    @app.template_filter()
    def format_time(dt):
        dt = pytz.UTC.localize(dt)
        dt = dt.astimezone(pytz.timezone('America/Chicago'))
        return dt.strftime('%Y-%-m-%d %-I:%M %p')

    @app.route("/")
    def index():
        scores = models.Score.query.order_by(models.Score.score.desc())
        return render_template("scores.html", scores=scores)

    @app.route("/api/v1/add_score", methods=('POST',))
    def add_score():
        # Lookup api key to find the Machine.
        auth_header = request.headers.get("Authorization")
        if not auth_header or not auth_header.startswith("Bearer "):
            abort(403)
        auth_token = auth_header.replace("Bearer ", "")

        try:
            machine = models.Machine.query.filter_by(auth_token=auth_token).one()
        except:
            abort(403)

        # Decode the request.
        # JSON data, then a NULL byte, then the payload
        data = request.data
        try:
            offset = data.index(0)
        except:
            abort(400)
        header = data[:offset].decode('utf-8')
        header = json.loads(header)
        body = data[(offset + 1):]

        if header.get("format") not in ["williams-sys7-v1", "williams-sys7-v2"]:
            print("Unsupported format: ", header.get("format"))
            abort(400)

        print("Got new scores! len=", len(body))
        game = models.Game(machine=machine, data=body)
        db.session.add(game)

        memory = System7Memory(body[-1024:]) # Use the last memory snapshot.
        for i, score in enumerate(memory.player_scores):
            print(f"  Player {i + 1}: {score}")
            entry = models.Score(
                machine=machine,
                game=game,
                player=(i + 1),
                score=score
            )
            db.session.add(entry)
        db.session.commit()

        return "ok"

    return app


app = create_app()
