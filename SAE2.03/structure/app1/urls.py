from django.urls import path
from . import views
from django.conf.urls.static import static
from django.conf import settings

urlpatterns = [
    # -------- ACCUEIL --------
    path('', views.index, name='index'),

    # -------- CLIENTS --------
    path('clients/', views.client_list, name='client-list'),
    path('clients/create/', views.client_form, name='client-create'),   # CHANGE ICI
    path('clients/<int:numero_client>/edit/', views.client_form, name='client-update'),  # CHANGE ICI
    path('clients/<int:numero_client>/delete/', views.client_delete, name='client-delete'),

    # -------- CATEGORIES --------
    path('categories/', views.categorie_grid, name='categorie-grid'),
    path('categories/manage/', views.categorie_manage, name='categorie-manage'),
    path('categories/create/', views.categorie_form, name='categorie-create'),   # CHANGE ICI
    path('categories/<int:pk>/', views.categorie_detail, name='categorie-detail'),
    path('categories/<int:pk>/edit/', views.categorie_form, name='categorie-update'),  # CHANGE ICI
    path('categories/<int:pk>/delete/', views.categorie_delete, name='categorie-delete'),

    # -------- PRODUITS --------
    path('produits/', views.produit_manage, name='produit-manage'),
    path('produits/create/', views.produit_form, name='produit-create'),   # CHANGE ICI
    path('produits/<int:pk>/edit/', views.produit_form, name='produit-update'),  # CHANGE ICI
    path('produits/<int:pk>/delete/', views.produit_delete, name='produit-delete'),

    # -------- PANIER ET COMMANDES ---------
    path("panier/", views.panier_view, name="panier"),
    path("commande/validation/", views.commande_validation, name="commande-validation"),
    path("commande/confirmation/<int:pk>/", views.commande_confirmation, name="commande-confirmation"),
    path('commandes/', views.commande_list, name='commande-list'),
    path('commandes/<int:pk>/delete/', views.commande_delete, name='commande-delete'),
]

if settings.DEBUG:
    urlpatterns += static(settings.MEDIA_URL, document_root=settings.MEDIA_ROOT)