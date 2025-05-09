import platform
from pathlib import Path
from script1 import open_directory_dialog
import json

directory = open_directory_dialog()

if not directory:
    print("Aucun répertoire sélectionné, fin du programme.")
    exit()

base_doss = Path(str(directory))

def listchem(doss):
    liste = doss.rglob("*")
    listtout = []
    for i in liste:
        if i.is_file():
            i = i.resolve()
            m = i.stat().st_size
            listtout.append([str(i), m])
    return listtout

def trie(listtout):
    listtout.sort(key=lambda x: x[1], reverse=True)
    for i in listtout:
        print(i)
    return listtout

def filtrer_fichiers(liste_triee, taille_mini_fichier_en_mo=1, nb_maxi_fichiers=100):
    liste_filtree = []
    for fichier in liste_triee:
        taille_en_mo = fichier[1] / 1048576  # Conversion de la taille en Mo
        if taille_en_mo >= taille_mini_fichier_en_mo:
            liste_filtree.append(fichier)
        else:
            continue

        if len(liste_filtree) == nb_maxi_fichiers:
            break
        else:
            continue

    return liste_filtree

def creer_fichier_json(liste_fichiers, fichier):
    for fichier_item in liste_fichiers:
        fichier_item[0] = fichier_item[0].replace('\\', '\\\\')
    with open(fichier, 'w', encoding='utf-8') as fichier_json:
        json.dump(liste_fichiers, fichier_json, indent=4, ensure_ascii=False)
    return fichier

liste = listchem(base_doss)
print("Fichiers trouvés :", liste)

if not liste:
    print("Aucun fichier trouvé dans le répertoire sélectionné.")
    exit()
liste_triee = trie(liste)

creer_fichier_json(liste_triee, "gros_fichiers.json")
print("Fichier JSON 'gros_fichiers.json' créé avec succès.")


