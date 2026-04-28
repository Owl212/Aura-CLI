#include <stdio.h>
#include <stdlib.h>

// Codes couleurs ANSI
#define COLOR_RESET   "\x1b[0m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_RED     "\x1b[31m"

void afficher_menu() {
    printf("\n");
    printf(COLOR_CYAN "=================================" COLOR_RESET "\n");
    printf(COLOR_CYAN "====      " COLOR_YELLOW "AURA-CLI MENU" COLOR_CYAN "      ====" COLOR_RESET "\n");
    printf(COLOR_CYAN "=================================" COLOR_RESET "\n");
    printf(COLOR_GREEN " 1." COLOR_RESET " Interroger l'API (Dev 2)\n");
    printf(COLOR_GREEN " 2." COLOR_RESET " Afficher le Leaderboard SQLite (Dev 1)\n");
    printf(COLOR_GREEN " 3." COLOR_RESET " Paramètres\n");
    printf(COLOR_RED " 0." COLOR_RESET " Quitter\n");
    printf(COLOR_CYAN "=================================" COLOR_RESET "\n");
    printf("Votre choix : ");
}

int main() {
    int choix;

    while (1) {
        afficher_menu();
        
        // Protection contre les entrées non-numériques
        if (scanf("%d", &choix) != 1) {
            while(getchar() != '\n'); // Vider le buffer
            printf(COLOR_RED "\n[Erreur] Entrée invalide. Veuillez entrer un nombre." COLOR_RESET "\n");
            continue;
        }

        switch (choix) {
            case 1:
                printf(COLOR_YELLOW "\n[Action] Requête API POST en cours avec libcurl..." COLOR_RESET "\n");
                // TODO: Appeler ici la fonction de l'API (libcurl / cJSON)
                break;
            case 2:
                printf(COLOR_YELLOW "\n[Action] Chargement de la base de données... (sqlite3)" COLOR_RESET "\n");
                // TODO: Appeler ici les fonctions init_db() et afficher_leaderboard()
                break;
            case 3:
                printf(COLOR_YELLOW "\n[Action] Ouverture des paramètres..." COLOR_RESET "\n");
                break;
            case 0:
                printf(COLOR_GREEN "\nFermeture d'Aura-CLI. Au revoir !" COLOR_RESET "\n");
                exit(0);
            default:
                printf(COLOR_RED "\n[Erreur] Choix inconnu. Veuillez réessayer." COLOR_RESET "\n");
                break;
        }
    }

    return 0;
}