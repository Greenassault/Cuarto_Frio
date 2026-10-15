from django.urls import path

from . import views

urlpatterns = [
    path("", views.panel, name="panel"),
    path("api/ultima/", views.ultima_lectura, name="ultima_lectura"),
    path("api/stream/", views.stream, name="stream"),
    path("historial/", views.historial, name="historial"),
    path("api/historial/", views.api_historial, name="api_historial"),
]