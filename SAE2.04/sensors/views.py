from django_filters.views import FilterView
from django_tables2.views import SingleTableMixin
from django.views.generic import TemplateView, UpdateView
from django.http import JsonResponse
from django.urls import reverse_lazy

from .models import Donnee, Capteur
from .tables import DonneeTable
from .filters import DonneeFilter

class DonneeListView(SingleTableMixin, FilterView):
    table_class = DonneeTable
    model = Donnee
    filterset_class = DonneeFilter
    template_name = "donnees/table.html"
    paginate_by = None

# --- Vue JSON pour le graphique

def serie_json(request, capteur_id):
    donnees = Donnee.objects.filter(capteur__id=capteur_id).order_by('-horodatage')[:20]
    donnees = list(reversed(donnees))  # pour avoir dans l’ordre chronologique


    labels = [d.horodatage.isoformat() for d in donnees]
    values = [float(d.temperature) for d in donnees]


    return JsonResponse({'labels': labels, 'values': values})


# --- Page HTML qui consomme la vue JSON

class GraphPage(TemplateView):
    template_name = "donnees/graph.html"

    def get_context_data(self, **kwargs):
        ctx = super().get_context_data(**kwargs)
        ctx["capteur_id"] = kwargs.get("capteur_id")
        return ctx

# --- Formulaire d’édition d’un capteur

class CapteurUpdateView(UpdateView):
    model = Capteur
    fields = ["nom", "emplacement"]
    template_name = "capteur/edit.html"
    success_url = reverse_lazy("donnees")