#--------------------------------------------------------
# Script : Creation_Onglets.py
# Destiné à la SAE 1.05 : traitement des données
# Dev : O. ECKLE - Ver : 1.0 - Décembre 2024
#--------------------------------------------------------

from PyQt5.QtWidgets import QWidget, QTabWidget, QVBoxLayout, QMainWindow

class Onglets(QMainWindow):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("Résultat de recherche des 'gros' fichiers...")
        self.setGeometry(200, 100, 1000, 700)
        self.onglets = QTabWidget()
        self.setCentralWidget(self.onglets)


    def add_onglet(self, titre, layout_a_ajouter):
        onglet = QWidget()
        layout_onglet = QVBoxLayout()
        layout_onglet.addWidget(layout_a_ajouter)
        onglet.setLayout(layout_onglet)
        self.onglets.addTab(onglet, titre)



