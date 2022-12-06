from flask_sqlalchemy import SQLAlchemy
from datetime import datetime, timedelta

db = SQLAlchemy()


class _TimeMixin:
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    updated_at = db.Column(
        db.DateTime, default=datetime.utcnow, onupdate=datetime.utcnow
    )


class Machine(db.Model, _TimeMixin):
    """
    A single pinball machine with a controller.
    """
    id = db.Column(db.Integer, primary_key=True)
    auth_token = db.Column(db.Text, nullable=True, index=True)
    name = db.Column(db.Text, nullable=False)
    kind = db.Column(db.Integer)


class Game(db.Model, _TimeMixin):
    id = db.Column(db.Integer, primary_key=True)
    machine_id = db.Column(db.Integer, db.ForeignKey("machine.id"), nullable=False)
    data = db.Column(db.LargeBinary)

    machine = db.relationship("Machine")


class Score(db.Model, _TimeMixin):
    id = db.Column(db.Integer, primary_key=True)
    machine_id = db.Column(db.Integer, db.ForeignKey("machine.id"), nullable=False)
    game_id = db.Column(db.Integer, db.ForeignKey("game.id"), nullable=False)
    player_num = db.Column(db.Integer)
    score = db.Column(db.Integer)
    player_name = db.Column(db.Text, nullable=True)

    machine = db.relationship("Machine")
    game = db.relationship("Game")

    @property
    def recent(self) -> bool:
        age = datetime.utcnow() - self.created_at
        return age < timedelta(hours=1)
