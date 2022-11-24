from flask_sqlalchemy import SQLAlchemy
from datetime import datetime

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


class RawScore(db.Model, _TimeMixin):
    id = db.Column(db.Integer, primary_key=True)
    machine_id = db.Column(db.Integer, db.ForeignKey("machine.id"), nullable=False)
    data = db.Column(db.LargeBinary)

    machine = db.relationship("Machine")


class Score(db.Model, _TimeMixin):
    id = db.Column(db.Integer, primary_key=True)
    machine_id = db.Column(db.Integer, db.ForeignKey("machine.id"), nullable=False)
    raw_score_id = db.Column(db.Integer, db.ForeignKey("raw_score.id"), nullable=False)
    player = db.Column(db.Integer)
    score = db.Column(db.Integer)

    machine = db.relationship("Machine")
    raw_score = db.relationship("RawScore")
