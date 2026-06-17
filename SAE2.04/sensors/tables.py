import django_tables2 as tables
from django_tables2.utils import A
from .models import Donnee


class DonneeTable(tables.Table):
    # lien clic‐able sur le nom du capteur → vue d’édition
    capteur_nom = tables.LinkColumn(
        "capteur_edit",                    # nom de l’URL
        args=[A("capteur.id")],            # pk du capteur
        accessor="capteur.nom",            # champ affiché
        verbose_name="Capteur"             # en-tête
    )

    # bouton « Graph » → vue graphique
    graph = tables.TemplateColumn(
        template_code="""
            <a href="{% url 'graph' capteur_id=record.capteur.id %}"
               class="btn btn-sm btn-outline-primary">
               Graph
            </a>
        """,
        orderable=False,
        verbose_name="Graph"
    )

    class Meta:
        model = Donnee
        template_name = "django_tables2/bootstrap4.html"   # inchangé
        # ordre et contenu des colonnes
        sequence = (
            "capteur.id",
            "capteur_nom",
            "capteur.piece",
            "capteur.emplacement",
            "horodatage",
            "temperature",
            "graph",
        )
        fields = sequence  # même liste, rien d’autre n’est touché
