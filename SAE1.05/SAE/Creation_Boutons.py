# --------------------------------------------------------
# Script : Creation_Boutons.py
# Destiné à la SAE 1.05 : traitement des données
# Dev : O. ECKLE - Ver : 1.0 - Décembre 2024
# --------------------------------------------------------

from PyQt5.QtWidgets import QWidget, QVBoxLayout, QPushButton, QLineEdit
from PyQt5.QtCore import Qt

class Boutons:
    """
    Création d'un objet graphique contenant un bouton et une textBox.
    Le chemin du répertoire de base est passé dans le 1er argument. Il est affiché dans la textBox.
    Le nom de la 'callcack' associé au 'clic' sur le bouton est passé en 2ème argument.
    """

    def __init__(self, repertoire_base, callback):
        self.repertoire_base = repertoire_base
        self.callback = callback

    def dessine_boutons(self):
        """
                Retourne une Widget Layout PyQt contenant un bouton et une textBox.
        """

        # Création de l'objet graphique et son layout contenant un bouton et une textbox
        boutons = QWidget()
        zone_boutons = QVBoxLayout(boutons)
        zone_boutons.setSpacing(10)
        zone_boutons.setAlignment(Qt.AlignTop)

        # Création du Bouton
        bouton_genere_delete_script = QPushButton("Créer le script pwsh de suppression des fichiers...")
        bouton_genere_delete_script.setFixedSize(500, 40)
        bouton_genere_delete_script.clicked.connect(self.callback)
        zone_boutons.addWidget(bouton_genere_delete_script, alignment=Qt.AlignHCenter)

        # Création de la textBox
        text_repertoire_base = QLineEdit()
        text_repertoire_base.setText(self.repertoire_base)
        text_repertoire_base.setFixedSize(490, 30)
        zone_boutons.addWidget(text_repertoire_base, alignment=Qt.AlignHCenter)

        # Retour de l'objet graphique et son layout contenant un bouton et une textBox
        return boutons


