from django import forms
from .models import Client, Categorie, Produit

class ClientForm(forms.ModelForm):
    class Meta:
        model = Client
        fields = ['nom', 'prenom', 'email', 'adresse']
        widgets = {
            'adresse': forms.Textarea(attrs={'rows': 2}),
        }



class CategorieForm(forms.ModelForm):
    class Meta:
        model = Categorie
        fields = ['nom', 'descriptif', 'image']   # Ajoute 'image' ici !
        widgets = {
            'descriptif': forms.Textarea(attrs={'rows': 2}),
        }

class ProduitForm(forms.ModelForm):
    class Meta:
        model = Produit
        fields = ['nom', 'date_peremption', 'photo', 'marque', 'prix', 'categorie']
        widgets = {
            'date_peremption': forms.DateInput(attrs={'type': 'date'}),
            'marque': forms.TextInput(),
        }