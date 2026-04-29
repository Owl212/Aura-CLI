#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "api.h"
#include "db.h"

// Pointeurs globaux pour l'interface GTK
GtkWidget *text_view;
GtkWidget *entry_nom;
GtkWidget *entry_reponse;
GtkTextBuffer *buffer;

// Fonction pour ajouter du texte dans la zone principale
void append_log(const gchar *text) {
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(buffer, &iter);
    gtk_text_buffer_insert(buffer, &iter, text, -1);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    
    // Auto-scroll en bas de la fenêtre
    GtkTextMark *mark = gtk_text_buffer_create_mark(buffer, NULL, &iter, FALSE);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(text_view), mark, 0.0, FALSE, 0.0, 0.0);
    gtk_text_buffer_delete_mark(buffer, mark);
    
    // Forcer l'affichage immédiat (utile avant un long blocage)
    while (gtk_events_pending()) gtk_main_iteration();
}

// Callback bouton "Envoyer"
static void on_envoyer_clicked(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    
    const gchar *nom = gtk_entry_get_text(GTK_ENTRY(entry_nom));
    const gchar *reponse = gtk_entry_get_text(GTK_ENTRY(entry_reponse));
    
    if (strlen(nom) == 0 || strlen(reponse) == 0) {
        append_log("[Système] Veuillez remplir votre nom et votre réponse.");
        return;
    }
    
    char log_msg[512];
    snprintf(log_msg, sizeof(log_msg), "[%s] Réponse : %s", nom, reponse);
    append_log(log_msg);
    append_log("> Analyse de votre réponse par l'IA Groq (Veuillez patienter...) ");
    
    char prompt_complet[1024];
    snprintf(prompt_complet, sizeof(prompt_complet),
             "L'étudiant a répondu : %s. Donne une note sur 10 et un court commentaire.", 
             reponse);

    // Blocage normal de libcurl - L'UI gèle pendant quelques secondes
    char *raw_json = ask_ai(prompt_complet);
    
    if (raw_json != NULL) {
        char *parsed_text = parse_ai_response(raw_json);
        if (parsed_text != NULL) {
            append_log("[IA Aura] :");
            append_log(parsed_text);
            free(parsed_text);
            
            int score_simule = rand() % 11;
            save_score(nom, score_simule);
            
            char score_msg[128];
            snprintf(score_msg, sizeof(score_msg), "[Système] Score aléatoire sauvé : %d/10 pour %s.", score_simule, nom);
            append_log(score_msg);
        } else {
            append_log("[Erreur] Impossible de lire la réponse IA.");
        }
        free(raw_json);
    } else {
         append_log("[Erreur API] Aucune connexion ou AURA_API_KEY manquante.");
    }
    
    // Vider le champ réponse
    gtk_entry_set_text(GTK_ENTRY(entry_reponse), "");
}

// Callback bouton "Historique" (Simplifié, on pourrait afficher dans le terminal ou textview)
static void on_historique_clicked(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    append_log("[Système] Affichage de l'historique dans la base de données :");
    // Dans une vraie appli, on réécrirait show_scores pour renvoyer une string.
    // Ici on affiche un message, show_scores() va printeur en arriere plan dans le terminal.
    append_log("(L'historique détaillé s'est affiché dans le terminal arrière-plan)");
    show_scores();
}

int main(int argc, char *argv[]) {
    // Protection API au lancement
    if (getenv("AURA_API_KEY") == NULL) {
        printf("Clé API manquante dans l'environnement\n");
        return 1;
    }

    init_db();

    // Initialisation GTK
    gtk_init(&argc, &argv);

    // Fenêtre principale
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Aura-CLI (Version Desktop)");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Conteneur Vertical Principal
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Zone de texte (Logs)
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scroll, TRUE);
    text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_container_add(GTK_CONTAINER(scroll), text_view);
    gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);

    // Champs nom
    entry_nom = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_nom), "Votre nom...");
    gtk_box_pack_start(GTK_BOX(vbox), entry_nom, FALSE, FALSE, 0);

    // Question
    GtkWidget *label_q = gtk_label_new("Question : Qu'est-ce qu'un pointeur en C ?");
    gtk_box_pack_start(GTK_BOX(vbox), label_q, FALSE, FALSE, 0);

    // Champ réponse
    entry_reponse = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_reponse), "Votre réponse ici...");
    gtk_box_pack_start(GTK_BOX(vbox), entry_reponse, FALSE, FALSE, 0);

    // Conteneur Horizontal pour les boutons
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    GtkWidget *btn_send = gtk_button_new_with_label("Envoyer à l'IA");
    g_signal_connect(btn_send, "clicked", G_CALLBACK(on_envoyer_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), btn_send, TRUE, TRUE, 0);

    GtkWidget *btn_hist = gtk_button_new_with_label("Voir l'Historique");
    g_signal_connect(btn_hist, "clicked", G_CALLBACK(on_historique_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), btn_hist, TRUE, TRUE, 0);

    // Message de bienvenue
    append_log("=== Bienvenue sur AURA Desktop ===");

    // Affichage
    gtk_widget_show_all(window);
    
    // Boucle d'événements principale
    gtk_main();

    return 0;
}
