import csv
from io import TextIOWrapper
from django.shortcuts import render, redirect
from .models import Client, Categorie, Produit, Commande
from .forms import ClientForm, CategorieForm, ProduitForm

# ==============================
#       ACCUEIL
# ==============================
def index(request):
    return render(request, 'app1/index.html')

# ==============================
#       CLIENTS - CRUD 
# ==============================
def client_list(request):
    q = request.GET.get('q', '')
    clients = Client.objects.all()
    if q:
        clients = clients.filter(nom__icontains=q) | clients.filter(prenom__icontains=q) | clients.filter(email__icontains=q)
    return render(request, "app1/clients.html", {'clients': clients, 'request': request})

def client_form(request, numero_client=None):
    instance = None
    if numero_client:
        try:
            instance = Client.objects.get(numero_client=numero_client)
            title = "Modifier"
            submit_label = "Enregistrer"
        except Client.DoesNotExist:
            return redirect('client-list')
    else:
        title = "Créer"
        submit_label = "Créer"
    form = ClientForm(request.POST or None, instance=instance)
    if request.method == "POST":
        if form.is_valid():
            form.save()
            return redirect('client-list')
    return render(request, 'app1/client_form.html', {
        "form": form,
        "title": title,
        "submit_label": submit_label,
    })

def client_delete(request, numero_client):
    try:
        client = Client.objects.get(numero_client=numero_client)
    except Client.DoesNotExist:
        return redirect('client-list')
    if request.method == 'POST':
        client.delete()
    return redirect('client-list')

# ==============================
#       CATEGORIES - CRUD 
# ==============================
def categorie_grid(request):
    categories = list(Categorie.objects.all())
    CELLS_TOTAL = 10
    empty_cells = CELLS_TOTAL - len(categories)
    empty_cells = max(0, empty_cells)
    return render(request, 'app1/categories_grid.html', {
        'categories': categories,
        'empty_cells': range(empty_cells),
    })

def categorie_manage(request):
    categories = Categorie.objects.all()
    return render(request, 'app1/categorie_manage.html', {'categories': categories})

def categorie_form(request, pk=None):
    instance = None
    if pk:
        try:
            instance = Categorie.objects.get(pk=pk)
            title = "Modifier"
            submit_label = "Enregistrer"
        except Categorie.DoesNotExist:
            return redirect('categorie-manage')
    else:
        title = "Créer"
        submit_label = "Créer"
    form = CategorieForm(request.POST or None, request.FILES or None, instance=instance)
    if request.method == "POST":
        if form.is_valid():
            form.save()
            return redirect('categorie-manage')
    return render(request, 'app1/categorie_form.html', {
        "form": form,
        "title": title,
        "submit_label": submit_label,
    })

def categorie_delete(request, pk):
    try:
        categorie = Categorie.objects.get(pk=pk)
    except Categorie.DoesNotExist:
        return redirect('categorie-manage')
    if request.method == "POST":
        categorie.delete()
    return redirect('categorie-manage')

def categorie_detail(request, pk):
    try:
        categorie = Categorie.objects.get(pk=pk)
    except Categorie.DoesNotExist:
        return redirect('categorie-manage')
    produits = Produit.objects.filter(categorie=categorie)
    panier = request.session.get('panier', {})
    if request.method == 'POST':
        produit_id = request.POST.get('produit_id')
        if produit_id:
            panier[produit_id] = panier.get(produit_id, 0) + 1
            request.session['panier'] = panier
            request.session.modified = True
            return redirect(request.path_info)
    panier_items = []
    total = 0
    for pid, qte in panier.items():
        try:
            produit = Produit.objects.get(id=pid)
            panier_items.append({'produit': produit, 'quantite': qte, 'total': produit.prix * qte})
            total += produit.prix * qte
        except Produit.DoesNotExist:
            continue
    q = request.GET.get('q')
    if q:
        produits = produits.filter(nom__icontains=q)
    return render(request, 'app1/categorie_detail.html', {
        'categorie': categorie,
        'produits': produits,
        'panier_items': panier_items,
        'total_panier': total,
    })

# ==============================
#       PRODUITS - CRUD 
# ==============================
def produit_manage(request):
    q = request.GET.get('q')
    categorie_id = request.GET.get('categorie')
    produits = Produit.objects.all()
    if categorie_id:
        produits = produits.filter(categorie_id=categorie_id)
    if q:
        produits = produits.filter(nom__icontains=q)
    return render(request, 'app1/produit_manage.html', {'produits': produits})

def produit_form(request, pk=None):
    instance = None
    import_message = None

    if not pk and request.method == "POST" and 'submit_csv' in request.POST:
        if request.FILES.get('fichier'):
            try:
                fichier = request.FILES['fichier']
                fichier_decoded = TextIOWrapper(fichier.file, encoding='utf-8')
                reader = csv.DictReader(fichier_decoded)
                cree, erreurs = 0, []
                for i, ligne in enumerate(reader, 2):
                    try:
                        nom = ligne['nom'].strip()
                        date_peremption = ligne['date_peremption'].strip()
                        marque = ligne.get('marque', '').strip() or None
                        prix = float(ligne['prix'])
                        cat_nom = ligne['categorie'].strip()
                        try:
                            categorie = Categorie.objects.get(nom__iexact=cat_nom)
                        except Categorie.DoesNotExist:
                            erreurs.append(f"Ligne {i}: Catégorie '{cat_nom}' inexistante")
                            continue
                        Produit.objects.create(
                            nom=nom,
                            date_peremption=date_peremption,
                            marque=marque,
                            prix=prix,
                            categorie=categorie,
                        )
                        cree += 1
                    except Exception as e:
                        erreurs.append(f"Ligne {i}: {e}")
                if cree:
                    import_message = f"{cree} produit(s) créés avec succès."
                if erreurs:
                    import_message = (import_message or '') + "<br>" + "<br>".join(erreurs)
            except Exception as err:
                import_message = f"Erreur de lecture du fichier: {err}"
        else:
            import_message = "Aucun fichier reçu."


    if pk:
        try:
            instance = Produit.objects.get(pk=pk)
            title = "Modifier"
            submit_label = "Enregistrer"
        except Produit.DoesNotExist:
            return redirect('produit-manage')
    else:
        title = "Créer"
        submit_label = "Créer"

    form = ProduitForm(request.POST or None, request.FILES or None, instance=instance)

  
    if request.method == "POST" and not ('submit_csv' in request.POST):
        if form.is_valid():
            form.save()
            return redirect('produit-manage')

    return render(request, 'app1/produit_form.html', {
        "form": form,
        "title": title,
        "submit_label": submit_label,
        "import_message": import_message,
        "is_update": bool(pk),
    })

def produit_delete(request, pk):
    try:
        produit = Produit.objects.get(pk=pk)
    except Produit.DoesNotExist:
        return redirect('produit-manage')
    if request.method == 'POST':
        produit.delete()
    return redirect('produit-manage')

# ========================
# PANIER
# ========================
def panier_view(request):
    panier = request.session.get('panier', {})
    if request.method == "POST":
        action = request.POST.get('action')
        produit_id = request.POST.get('produit_id')
        if produit_id and produit_id in panier:
            if action == "plus":
                panier[produit_id] += 1
            elif action == "moins" and panier[produit_id] > 1:
                panier[produit_id] -= 1
            elif action == "set":
                try:
                    new_qte = int(request.POST.get('quantite', 1))
                    panier[produit_id] = max(1, new_qte)
                except ValueError:
                    pass
            elif action == "remove":
                del panier[produit_id]
            request.session['panier'] = panier
            request.session.modified = True
        if "valider" in request.POST:
            return redirect('commande-validation')

    panier_items, total = [], 0
    for pid, qte in panier.items():
        try:
            produit = Produit.objects.get(id=pid)
            panier_items.append({
                'produit': produit,
                'quantite': qte,
                'total': produit.prix * qte
            })
            total += produit.prix * qte
        except Produit.DoesNotExist:
            continue
    return render(request, "app1/panier.html", {
        "panier_items": panier_items,
        "total_panier": total,
    })


# =================================
#  COMMANDE
# =================================

from django.shortcuts import render, redirect
from .models import Client, Produit, Commande, LigneCommande

def commande_validation(request):
    panier = request.session.get('panier', {})
    if not panier:
        return redirect('panier')
    panier_items = []
    total = 0
    for pid, qte in panier.items():
        try:
            produit = Produit.objects.get(id=pid)
            panier_items.append({'produit': produit, 'quantite': qte, 'total': produit.prix * qte})
            total += produit.prix * qte
        except Produit.DoesNotExist:
            continue
    clients = Client.objects.all()
    if request.method == "POST":
        client_id = request.POST.get('client_id')
        if client_id and client_id.isdigit():
            client = Client.objects.get(pk=int(client_id))
            commande = Commande.objects.create(client=client)
            for item in panier_items:
                LigneCommande.objects.create(
                    commande=commande,
                    produit=item['produit'],
                    quantite=item['quantite']
                )
            del request.session['panier']
            return redirect('commande-confirmation', pk=commande.numero_commande)
    return render(request, "app1/commande_validation.html", {
        "panier_items": panier_items,
        "total_panier": total,
        "clients": clients,
    })
def commande_confirmation(request, pk):
    commande = Commande.objects.filter(pk=pk).first()
    return render(request, "app1/commande_confirmation.html", {"commande": commande})

def commande_list(request):
    commandes = Commande.objects.select_related('client').prefetch_related('lignes__produit').order_by('-date')
    return render(request, 'app1/commande_list.html', {
        "commandes": commandes
    })

def commande_delete(request, pk):
    try:
        commande = Commande.objects.get(pk=pk)
    except Commande.DoesNotExist:
        return redirect('commande-list')
    if request.method == "POST":
        commande.delete()
    return redirect('commande-list')