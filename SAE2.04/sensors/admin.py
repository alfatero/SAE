
from django.contrib import admin
from .models import Capteur, Donnee

@admin.register(Capteur)
class CapteurAdmin(admin.ModelAdmin):
    list_display = ("id", "nom", "piece", "emplacement")
    search_fields = ("id", "nom")

@admin.register(Donnee)
class DonneeAdmin(admin.ModelAdmin):
    list_display = ("capteur", "horodatage", "temperature")
    list_filter = ("capteur",)
    date_hierarchy = "horodatage"