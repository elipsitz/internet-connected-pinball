from flask import Flask, request, abort, render_template, send_file
import json
import os
import pytz

from . import admin, models
from .models import db
from .memory import System7Memory, System7Game

instance_path = os.environ.get("INSTANCE_PATH", "/data")
app = Flask(__name__, instance_path=instance_path)
app.config.from_mapping(
    SQLALCHEMY_TRACK_MODIFICATIONS=False,
    SQLALCHEMY_DATABASE_URI="sqlite:///" + os.path.join(app.instance_path, "db.sqlite"),
    SQLALCHEMY_ECHO=app.config["DEBUG"],
)
with open(os.path.join(instance_path, "config.json")) as f:
    app.config.from_mapping(json.load(f))
db.init_app(app)
admin.init_app(app)

@app.template_filter()
def commaify(value):
    return "{:,}".format(value)

@app.template_filter()
def format_time(dt):
    dt = pytz.UTC.localize(dt)
    dt = dt.astimezone(pytz.timezone('America/Chicago'))
    return dt.strftime('%Y-%-m-%d %-I:%M %p')

@app.route("/robots.txt")
def robots_txt():
    return send_file("static/robots.txt")

@app.route("/")
def index():
    valid_keys = ("created_at", "score", "player_name")
    valid_orders = ("desc", "asc")

    sort_key = "score"
    sort_order = "desc"
    sort_key = request.args.get("sort") or sort_key
    sort_order = request.args.get("order") or sort_order

    if sort_key in valid_keys and sort_order in valid_orders:
        sort = getattr(getattr(models.Score, sort_key), sort_order)()
    else:
        sort = models.Score.score.desc()

    scores = models.Score.query.order_by(sort)
    return render_template(
        "scores.html",
        scores=scores,
        sort_key=sort_key,
        sort_order=sort_order
    )

@app.route("/game/<int:game_id>")
def show_game(game_id):
    model = models.Game.query.get_or_404(game_id)
    game = System7Game(model.data)
    return render_template("game.html", model=model, game=game)

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
            player_num=(i + 1),
            score=score
        )
        db.session.add(entry)
    db.session.commit()

    return "ok"

@app.route("/api/v1/set_player_name", methods=('POST',))
def set_player_name():
    score_id = request.form["score_id"]
    player_name = request.form["player_name"].strip()[:3]
    score = models.Score.query.get_or_404(score_id)
    if (score.player_name is not None) or (not score.recent):
        abort(400)
    score.player_name = player_name
    db.session.commit()
    return "ok"
