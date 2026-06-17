import django_filters as df
from .models import Donnee, Capteur


class DonneeFilter(df.FilterSet):
    # 🔽 Capteur : liste déroulante basée sur le modèle
    capteur = df.ModelChoiceFilter(
        field_name="capteur",
        queryset=Capteur.objects.all(),
        label="Capteur",
        empty_label="Tous",
    )

    # 🔽 Pièce : liste déroulante dynamique (valeurs distinctes)
    piece = df.ChoiceFilter(
        field_name="capteur__piece",
        label="Pièce",
        choices=lambda: (
            Donnee.objects
                  .values_list("capteur__piece", "capteur__piece")
                  .distinct()
                  .order_by("capteur__piece")
        ),
        empty_label="Toutes",
    )

    # 📅 Période : deux date-pickers
    date = df.DateFromToRangeFilter(
        field_name="horodatage",
        label="Période",
        widget=df.widgets.RangeWidget(attrs={"type": "date"}),
    )

    # 🌡️ Température : plage min / max
    temperature = df.RangeFilter(
        field_name="temperature",
        label="Température (°C)",
        widget=df.widgets.RangeWidget(attrs={"type": "number", "step": "0.1"}),
    )

    class Meta:
        model  = Donnee
        fields = []        # on pilote explicitement les filtres
