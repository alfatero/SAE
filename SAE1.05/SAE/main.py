import os
from script2 import listchem, trie, creer_fichier_json, base_doss, filtrer_fichiers



liste = listchem(base_doss)

print("Liste des fichiers trouvés :", liste)

print("\nTri des fichiers par taille décroissante")
liste_triee = trie(liste)

fichiers_filtrés = filtrer_fichiers(liste_triee)
print("\nFichiers filtrés :", fichiers_filtrés)

nom_fichier_json = "gros_fichiers.json"
print("\nCréation du fichier JSON...")

creer_fichier_json(fichiers_filtrés, nom_fichier_json)
print(f"Le fichier JSON '{nom_fichier_json}' a été créé ")
print(f"Nombre de fichiers dans le JSON : {len(fichiers_filtrés)}")

print("\nLancement de l'interface graphique")
os.system("python script3.py")
