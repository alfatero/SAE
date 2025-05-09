Write-Output "Script PowerShell pour supprimer les fichiers sélectionnés"
Write-Output "Attention : cette suppression est définitive..."
$reponse = Read-Host "Confirmez la suppression des fichiers (OUI)"
if ($reponse -eq "OUI") {
    $confirmation = Read-Host "Êtes-vous sûr ? (OUI)"
    if ($confirmation -eq "OUI") {
        Remove-Item -Path "C:\\Users\\e2400612\\Desktop\\202501271654EVE-NG_SAE12\\EVE-NG-disk1.vmdk" -Force
    } else { Write-Output "Opération annulée..." }
} else { Write-Output "Opération annulée..." }