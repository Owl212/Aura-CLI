#include <stdio.h>
#include <stdlib.h>

// Codes couleurs ANSI
#define COLOR_RESET   "\x1b[0m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_GREEN   "\x1b[32m"

void afficher_menu() {
    printf("\n");
    // Titre en ASCII Art 'AURA-CLI' avec couleur Cyan
    printf(COLOR_CYAN);
    printf("    ___   __  __  ____    ___         ____  __    ____ \n");
    printf("   /   | / / / / / __ \\  /   |       / ___|/ /   /  _/ \n");
    printf("  / /| |/ / / / / /_/ / / /| | _____/ /   / /    / /   \n");
    printf(" / ___ / /_/ / / _, _/ / ___ |/____/ /___/ /____/ /    \n");
    printf("/_/  |_\\____/ /_/ |_| /_/  |_|     \\____/_____/___/    \n");
    printf(COLOR_RESET);
    
    printf("\n=========================================\n");
    printf(" [1] Démarrer un entretien\n");
    printf(" [2] Voir l'historique\n");
    printf(" [3] Quitter\n");
    printf("=========================================\n");
    printf("Votre choix : ");
}

int main() {
    char choix_str[10];

    while (1) {
        afficher_menu();
        
        // Utilisation de fgets pour lire l'entrée proprement
        if (fgets(choix_str, sizeof(choix_str), stdin) != NULL) {
            int choix = atoi(choix_str);
            
            switch (choix) {
                case 1:
                    printf(COLOR_YELLOW "\nFonctionnalite en cours de developpement...\n" COLOR_RESET);
                    break;
                case 2:
                    printf(COLOR_YELLOW "\nFonctionnalite en cours de developpement...\n" COLOR_RESET);
                    break;
                case 3:
                    printf(COLOR_GREEN "\nAu revoir !\n" COLOR_RESET);
                    return 0; // ou exit(0);
                default:
                    printf("\nChoix invalide. Veuillez reessayer.\n");
                    break;
            }
        }
    }

    return 0;
}