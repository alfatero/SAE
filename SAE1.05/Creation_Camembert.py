#--------------------------------------------------------
# Script : Creation_Camenbert.py
# Destiné à la SAE 1.05 : traitement des données
# Dev : O. ECKLE - Ver : 1.0 - Décembre 2024
#--------------------------------------------------------

from PyQt5.QtChart import QChart, QChartView, QPieSeries
from PyQt5.QtGui import QFont

class Camembert:
    """
    Création d'un objet graphique contenant une réprésentation statistique sous forme de camembert.
    La série de données représente la liste passée dans le 1er argument.
    Les couleurs des tranches du camembert sont celle de la liste passée en 2ème argument.
    """
    def __init__(self, liste_fichiers, liste_couleurs):
        self.liste_fichiers = liste_fichiers
        self.liste_couleurs = liste_couleurs

    def dessine_camembert(self):
        """
        Retourne une Widget Layout PyQt contenant un graphique circulaire type camembert.
        """
        if not self.liste_fichiers:
            raise ValueError(f"La liste doit contenir au moins 1 fichier.")

        # Création de la série de données pour le graphique
        series = QPieSeries()
        series.setLabelsVisible(True)
        taille_totale = 0
        for liste in self.liste_fichiers:
            taille_totale += liste[1]

        # Création des différentes tranches du camembert
        font = QFont("Arial Narrow", 12, QFont.Bold)
        for path_fichier, taille_fichier in self.liste_fichiers:
            etiquette = f"{taille_fichier // 1048576}Mo"
            pourcentage = taille_fichier / taille_totale * 100
            slice_ = series.append(etiquette, pourcentage)
            slice_.setBrush(self.liste_couleurs[len(series)-1])  # Couleurs dynamiques
            slice_.setLabelFont(font)
            slice_.setLabelPosition(slice_.LabelPosition.LabelOutside)

        # Affichage d'une étiquette pour les grandes tranches du camembert
        for slice_ in series.slices():
            slice_.setLabelVisible(slice_.angleSpan() > 6)

        # Création du camembert complet
        fromage = QChart()
        fromage.addSeries(series)
        fromage.setTitle("Répartition des tailles des fichiers")
        fromage.legend().hide()

        # Configuration du graphique avec QChartView
        layout_fromage = QChartView(fromage)

        # Retour de l'objet graphique et son layout contenant le camembert
        return layout_fromage


