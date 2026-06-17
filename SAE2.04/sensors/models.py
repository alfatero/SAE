from django.db import models

class Capteur(models.Model):
    id = models.CharField(primary_key=True, max_length=20)
    piece = models.CharField(max_length=50)
    nom = models.CharField(max_length=50, unique=True)
    emplacement = models.CharField(max_length=100, blank=True, null=True)

    def __str__(self):
        return self.nom or self.id

    class Meta:
        db_table = "capteur"
        managed = False

class Donnee(models.Model):
    capteur = models.ForeignKey(Capteur, on_delete=models.CASCADE, db_column="capteur_id", primary_key=True,)
    horodatage = models.DateTimeField(db_index=True)
    temperature = models.DecimalField(max_digits=5, decimal_places=2)

    class Meta:
        db_table = "donnees"
        managed = False
        ordering = ["-horodatage"]
        unique_together = ("capteur", "horodatage")  # évite les doublons

