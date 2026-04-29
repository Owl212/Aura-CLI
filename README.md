# Aura CLI (Desktop Edition)

Aura est un projet d'interrogation et d'évaluation par Intelligence Artificielle (via l'API Groq). Il permet à un utilisateur de saisir une réponse à une question technique (ex: programmation C) et d'obtenir une note ainsi qu'un commentaire de l'IA. Les scores sont sauvegardés via une base de données SQLite.

## ⚠️ ATTENTION - PRÉREQUIS GTK3 (IMPORTANT POUR LA COMPILATION) ⚠️

Ce projet a été **migré d'une application Terminal (CLI) vers une véritable Interface Graphique de Bureau (Desktop)** utilisant **GTK3**. 
Par conséquent, **le code ne compilera pas du tout** si ces bibliothèques graphiques (et autres dépendances) ne sont pas installées sur votre système !

### 🛠️ Installation des dépendances sous MSYS2 / MinGW64 (Windows)

Pour permettre au compilateur (`gcc`) de trouver l'interface graphique GTK3, les requêtes réseau (`libcurl`), et la base de données (`sqlite3`), veuillez ouvrir le terminal **MSYS2 MinGW 64-bit** (et non pas un terminal Windows standard) et exécuter impérativement la commande suivante pour installer tous les paquets nécessaires :

```bash
pacman -S --noconfirm mingw-w64-x86_64-gtk3 mingw-w64-x86_64-pkg-config mingw-w64-x86_64-curl mingw-w64-x86_64-sqlite3 mingw-w64-x86_64-gcc make
```

**Explications des paquets :**
- `gtk3` / `pkg-config` : Permettent de générer et lier la fenêtre de l'interface graphique Windows.
- `curl` : Permet la communication TCP/HTTP avec l'API IA (Groq).
- `sqlite3` : Le moteur de base de données local.

### 🚀 Exécution de l'Application

1. **Configuration du PATH (Si vous utilisez VS Code ou PowerShell) :**
   Avant de lancer la compilation, assurez-vous que votre terminal peut voir les outils MSYS2 :
   ```powershell
   $env:Path += ";C:\msys64\usr\bin;C:\msys64\mingw64\bin"
   ```

2. **Clé API (Obligatoire) :**
   Vous devez définir la clé d'accès à l'API IA dans l'environnement avant de lancer l'exécutable.
   ```powershell
   $env:AURA_API_KEY="votre_cle_api_groq"
   ```

3. **Compiler le projet :**
   ```bash
   make clean
   make
   ```

4. **Lancer l'interface :**
   ```bash
   ./bin/aura_cli.exe
   ```
