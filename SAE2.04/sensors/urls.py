from django.urls import path
from . import views

urlpatterns = [
    path("", views.DonneeListView.as_view(), name="donnees"),
    path("graph/<str:capteur_id>/data/", views.serie_json, name="serie_json"),
    path("graph/<str:capteur_id>/", views.GraphPage.as_view(), name="graph"),
    path("capteur/<str:pk>/edit/", views.CapteurUpdateView.as_view(), name="capteur_edit"),
]

