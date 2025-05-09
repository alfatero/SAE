from PyQt5.QtWidgets import QWidget, QVBoxLayout, QLabel, QHBoxLayout, QCheckBox
from PyQt5.QtCore import Qt

class Legendes:
    def __init__(self, liste_fichiers, liste_couleurs, num_legende_start, nb_legende_par_page):
        self.liste_fichiers = liste_fichiers
        self.liste_couleurs = liste_couleurs
        self.num_legende_start = num_legende_start
        self.nb_legende_par_page = nb_legende_par_page
        self.num_legende_stop = min(len(liste_fichiers), num_legende_start+self.nb_legende_par_page)
        self.cases_a_cocher = []

    def dessine_legendes(self):
        legendes = QWidget()
        zone_legendes = QVBoxLayout(legendes)
        zone_legendes.setContentsMargins(0, 20, 0, 20)
        zone_legendes.setAlignment(Qt.AlignTop)

        for num_slice in range(self.num_legende_start, self.num_legende_stop):
            case_a_cocher = QCheckBox()
            case_a_cocher.setChecked(False)
            self.cases_a_cocher.append(case_a_cocher)

            rectangle_colore = QWidget()
            rectangle_colore.setFixedSize(16, 16)
            couleur = self.liste_couleurs[num_slice].name()
            rectangle_colore.setStyleSheet(f"background-color: {couleur}; border: 1px solid black;")

            chemin_fichier = self.liste_fichiers[num_slice][0]
            taille_fichier = self.liste_fichiers[num_slice][1] // 1048576
            etiquette_legende = f"<span style='color:black;font-family:Arial Narrow'>{chemin_fichier} --> (</span><span style='color:red;'>{taille_fichier} Mo</span>)"
            texte_legende = QLabel(etiquette_legende)

            ligne_legende = QHBoxLayout()
            ligne_legende.setAlignment(Qt.AlignLeft)
            ligne_legende.setContentsMargins(5, 0, 5, 5)
            ligne_legende.stretch(2)
            ligne_legende.addWidget(case_a_cocher)
            ligne_legende.addWidget(rectangle_colore)
            ligne_legende.addWidget(texte_legende)

            legende = QWidget()
            legende.setLayout(ligne_legende)
            legende.setContentsMargins(0, 0, 0, 0)

            zone_legendes.addWidget(legende)
            zone_legendes.setSpacing(0)

        return legendes

    def recupere_etats_cases_a_cocher(self):
        etats = []
        for case in self.cases_a_cocher:
            etats.append(case.isChecked())
        return etats
