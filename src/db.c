#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include "db.h"

// Le fichier de base de données sera dans le dossier "data/"
const char* db_path = "data/local.db";

// =====================================================================
// EXPLICATION SOUTENANCE : init_db()
// =====================================================================
// SQLite crée ou ouvre un système de fichiers local via sqlite3_open().
// 
// Mécanisme : 
// - db_path contient le texte désignant le chemin vers notre base "data/local.db".
// - `sqlite3 *db` : Pointeur qui contiendra l'instance connectée de SQLite.
// On passe son adresse (&db) en paramètre pour que la librairie SQLite
// y affecte le pointeur réel.
// 
// Création via sql brut : `sqlite3_exec` exécute du texte SQL brut car
// cette commande "CREATE TABLE" n'implique pas de saisies utilisateurs
// dangereuses (donc pas de risque d'injection SQL ici).
// =====================================================================
void init_db() {
    sqlite3 *db;
    char *err_msg = 0;
    
    // Ouvre (ou crée) la base de données
    int rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Erreur d'ouverture de la BDD: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }
    
    // Création de la table si elle n'existe pas
    const char *sql = "CREATE TABLE IF NOT EXISTS interviews ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "user_name TEXT NOT NULL,"
                      "score INTEGER NOT NULL);";
    
    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK ) {
        fprintf(stderr, "Erreur SQL: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
    
    sqlite3_close(db);
}

// =====================================================================
// EXPLICATION SOUTENANCE : save_score() 
// =====================================================================
// Cette fonction protège contre les failles d'injections SQL grâce 
// aux "Prepared Statements" (Requêtes préparées).
// 
// Mécanisme :
// 1. `sqlite3_prepare_v2` : Analyse syntaxiquement la requête sans l'exécuter.
//    Ceci renvoie un objet "statement" (`sqlite3_stmt *stmt`).
// 2. `sqlite3_bind_*` : Sécurise et remplace les "?" de la requête par les 
//    vraies valeurs (le nom et le score). SQLite va échapper automatiquement
//    les caractères malveillants fournis dans l'interface utilisateur.
// 3. `sqlite3_step` : Exécute concrètement la requête verrouillée sur le disque.
// 4. `sqlite3_finalize(stmt)` : Supprime expressément l'objet statement local
//    en mémoire vive (indispensable pour éviter les memory leaks et les crashs).
// =====================================================================
void save_score(const char* user_name, int score) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    
    int rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Erreur d'ouverture de la BDD: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }
    
    // Utilisation de requêtes préparées pour éviter les failles d'injection SQL
    const char *sql = "INSERT INTO interviews(user_name, score) VALUES(?, ?);";
    
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        // Lier les variables (1 = user_name, 2 = score)
        sqlite3_bind_text(stmt, 1, user_name, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, score);
        
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            fprintf(stderr, "Erreur lors de l'insertion: %s\n", sqlite3_errmsg(db));
        }
        
        // Libération IMPÉRATIVE de la mémoire du "statement"
        sqlite3_finalize(stmt);
    } else {
        fprintf(stderr, "Erreur de préparation SQL: %s\n", sqlite3_errmsg(db));
    }
    
    sqlite3_close(db);
}

// =====================================================================
// EXPLICATION SOUTENANCE : show_scores()
// =====================================================================
// Une requête "SELECT" permet de chercher des données écrites dans la base.
// 
// Mécanisme de boucle et extraction :
// - `sqlite3_step(stmt)` est appelée dans une boucle `while()`. 
//   Elle avancera de "ligne en ligne" (Row by Row) à chaque itération.
// - `sqlite3_column_int` et `sqlite3_column_text` extraient précisément
//   les données des colonnes respectives indexées (0, 1, 2 = id, user_name, score).
// - Le pointeur texte `(const unsigned char *name)` retourné pointe directement
//   dans le flux mémoire du "statement". Il ne doit exister aucune allocation manuelle ici,
//   mais ce pointeur devient obsolète juste après `sqlite3_finalize(stmt)`.
// =====================================================================
void show_scores() {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    
    int rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Erreur d'ouverture de la BDD: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }
    
    printf("\n" "\x1b[36m" "=========================================" "\x1b[0m" "\n");
    printf(" | " "\x1b[33m" "ID" "\x1b[0m" "   | " "\x1b[33m" "UTILISATEUR" "\x1b[0m" "      | " "\x1b[33m" "SCORE" "\x1b[0m" " |\n");
    printf("-----------------------------------------\n");
    
    const char* sql = "SELECT id, user_name, score FROM interviews;";
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    
    if (rc == SQLITE_OK) {
        // Boucle pour lire proprement chaque ligne retournée
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            const unsigned char *name = sqlite3_column_text(stmt, 1);
            int score = sqlite3_column_int(stmt, 2);
            
            printf(" | %-4d | %-16s | %-5d |\n", id, name ? (const char*)name : "INCONNU", score);
        }
        
        // Libération IMPÉRATIVE de la mémoire du "statement"
        sqlite3_finalize(stmt);
    } else {
        fprintf(stderr, "Erreur SELECT SQL: %s\n", sqlite3_errmsg(db));
    }
    
    printf("\x1b[36m" "=========================================" "\x1b[0m" "\n");
    
    sqlite3_close(db);
}