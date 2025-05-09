import json
import sys
import random
from PyQt5.QtWidgets import QApplication
from PyQt5.QtGui import QColor
from Creation_Onglets import Onglets
from Creation_Camembert import Camembert
from Creation_Legendes import Legendes
from Creation_Boutons import Boutons

NB_LEGENDES_PAR_PAGE = 25

def lire_fichier_json(nom_fichier):
    with open(nom_fichier, 'r', encoding='utf-8') as fichier_json:
        return json.load(fichier_json)

def generer_couleurs(nb):
    return [QColor(random.randint(0, 255), random.randint(0, 255), random.randint(0, 255)) for _ in range(nb)]

def generer_script_suppression(fichiers_a_supprimer):
    script_ps = """Write-Output "Script PowerShell pour supprimer les fichiers sélectionnés"
Write-Output "Attention : cette suppression est définitive..."
$reponse = Read-Host "Confirmez la suppression des fichiers (OUI)"
if ($reponse -eq "OUI") {
    $confirmation = Read-Host "Êtes-vous sûr ? (OUI)"
    if ($confirmation -eq "OUI") {\n"""
    
    for fichier in fichiers_a_supprimer:
        script_ps += f'        Remove-Item -Path "{fichier}" -Force\n'
    
    script_ps += """    } else { Write-Output "Opération annulée..." }
} else { Write-Output "Opération annulée..." }"""

    with open("suppression_fichiers.ps1", "w", encoding="utf-8") as script_file:
        script_file.write(script_ps)
    
    print("Script PowerShell 'suppression_fichiers.ps1' généré avec succès !")

def callback_suppression(liste_legende):
    fichiers_a_supprimer = []
    for legende in liste_legende:
        etats_cases = legende.recupere_etats_cases_a_cocher()
        for i, etat in enumerate(etats_cases):
            if etat:
                fichiers_a_supprimer.append(legende.liste_fichiers[i][0])
    
    if fichiers_a_supprimer:
        generer_script_suppression(fichiers_a_supprimer)
    else:
        print("Aucun fichier sélectionné pour la suppression.")

if __name__ == "__main__":
    nom_fichier_json = "gros_fichiers.json"
    liste_fichiers = lire_fichier_json(nom_fichier_json)
    
    if not liste_fichiers:
        print("Le fichier JSON est vide ou n'existe pas.")
        sys.exit(1)

    liste_couleurs = generer_couleurs(len(liste_fichiers))

    app = QApplication(sys.argv)
    fenetre = Onglets()

    camembert = Camembert(liste_fichiers, liste_couleurs)
    layout_camembert = camembert.dessine_camembert()
    fenetre.add_onglet("Camembert", layout_camembert)

    liste_legende = []
    for num_page in range(0, len(liste_fichiers), NB_LEGENDES_PAR_PAGE):
        legende = Legendes(
            liste_fichiers[num_page:num_page+NB_LEGENDES_PAR_PAGE], 
            liste_couleurs, 
            num_page, 
            NB_LEGENDES_PAR_PAGE
        )
        layout_legende = legende.dessine_legendes()
        fenetre.add_onglet(f"Légende {num_page // NB_LEGENDES_PAR_PAGE + 1}", layout_legende)
        liste_legende.append(legende)

    ihm = Boutons(nom_fichier_json, lambda: callback_suppression(liste_legende))
    layout_ihm = ihm.dessine_boutons()
    fenetre.add_onglet("IHM", layout_ihm)

    fenetre.show()
    sys.exit(app.exec_())