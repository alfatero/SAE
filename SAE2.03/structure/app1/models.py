from django.db import models

# Create your models here.

class Categorie(models.Model):
    nom = models.CharField(max_length=100, unique=True)
    descriptif = models.TextField(blank=True)
    image = models.ImageField(upload_to='categories/')  

    def __str__(self):
        return self.nom

class Produit(models.Model):
    nom = models.CharField(max_length=200)
    date_peremption = models.DateField()
    photo = models.ImageField(upload_to='photos_produits/', blank=True, null=True)
    marque = models.CharField(max_length=100, blank=True, null=True)
    prix = models.DecimalField(max_digits=8, decimal_places=2)
    categorie = models.ForeignKey(Categorie, on_delete=models.CASCADE, related_name='produits')

    def __str__(self):
        return f"{self.nom} ({self.marque})"

class Client(models.Model):
    numero_client = models.AutoField(primary_key=True)
    nom = models.CharField(max_length=200)
    prenom = models.CharField(max_length=200)
    email = models.EmailField(unique=True)  
    date_inscription = models.DateField(auto_now_add=True)
    adresse = models.TextField()

    def __str__(self):
        return f"{self.prenom} {self.nom}"

class Commande(models.Model):
    numero_commande = models.AutoField(primary_key=True)
    client = models.ForeignKey(Client, on_delete=models.CASCADE, related_name='commandes')
    date = models.DateTimeField(auto_now_add=True)

    def __str__(self):
        return f"Commande #{self.numero_commande}"

    def cout_total(self):
        # renvoie le coût total de la commande
        return sum([ligne.produit.prix * ligne.quantite for ligne in self.lignes.all()])

class LigneCommande(models.Model):
    commande = models.ForeignKey(Commande, related_name='lignes', on_delete=models.CASCADE)
    produit = models.ForeignKey(Produit, on_delete=models.CASCADE)
    quantite = models.PositiveIntegerField()

    class Meta:
        unique_together = ('commande', 'produit')