from flask import url_for, Markup, escape
from flask_admin import Admin, AdminIndexView, BaseView, expose
from flask_admin.contrib.sqla import ModelView
from sqlalchemy.orm.query import Query
from functools import wraps
from flask import current_app, request, Response
from werkzeug.security import check_password_hash

from . import models

def is_admin():
    auth = request.authorization
    if not auth:
        return False
    admin_username = current_app.config["ADMIN_USERNAME"]
    admin_password = current_app.config["ADMIN_PASSWORD"]
    return auth.username == admin_username and check_password_hash(
        admin_password, auth.password
    )


def admin_required_response():
    if not is_admin():
        response = Response(
            "<h1>Unauthorized</h1>",
            401,
            {"WWW-Authenticate": 'Basic realm="Login Required"'},
        )
        return response


def admin_required(f):
    @wraps(f)
    def decorated_function(*args, **kwargs):
        return admin_required_response() or f(*args, **kwargs)

    return decorated_function


class SecuredView(BaseView):
    def is_accessible(self):
        return is_admin()

    def inaccessible_callback(self, name, **kwargs):
        return admin_required_response()


class SecuredHomeView(AdminIndexView, SecuredView):
    @expose("/")
    def index(self):
        return self.render("admin/master.html")


class BaseModelView(ModelView, SecuredView):
    pass


class MachineView(BaseModelView):
    can_view_details = True
    can_create = True
    can_delete = True
    can_edit = True

    named_filter_urls = True


class GameView(BaseModelView):
    can_view_details = True
    can_create = True
    can_delete = True
    can_edit = True

    column_exclude_list = ["data"]
    named_filter_urls = True


class ScoreView(BaseModelView):
    can_view_details = True
    can_create = True
    can_delete = True
    can_edit = True

    named_filter_urls = True


def init_app(app):
    admin = Admin(template_mode="bootstrap3", index_view=SecuredHomeView())
    admin.add_view(MachineView(models.Machine, models.db.session))
    admin.add_view(GameView(models.Game, models.db.session))
    admin.add_view(ScoreView(models.Score, models.db.session))
    admin.init_app(app)
