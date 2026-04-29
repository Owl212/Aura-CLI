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
GtkWidget *spinner;
GtkWidget *btn_send;

// Fonction pour ajouter du texte dans la zone principale
void append_log(const gchar *text) {
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(buffer, &iter);
    gtk_text_buffer_insert(buffer, &iter, text, -1);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    
    GtkTextMark *mark = gtk_text_buffer_create_mark(buffer, NULL, &iter, FALSE);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(text_view), mark, 0.0, FALSE, 0.0, 0.0);
    gtk_text_buffer_delete_mark(buffer, mark);
}

// --- Structures pour le Threading ---
typedef struct {
    gchar *nom;
    gchar *prompt_complet;
} AskAIData;

typedef struct {
    gchar *nom;
    gchar *parsed_text;
    gchar *error_msg;
} ResultData;

// Fonction exécutée dans le thread principal (UI) après le thread réseau
static gboolean update_ui_after_ai(gpointer user_data) {
    ResultData *res = (ResultData *)user_data;

    if (res->parsed_text) {
        append_log("[IA Aura] :");
        append_log(res->parsed_text);
        
        int score_simule = rand() % 11;
        save_score(res->nom, score_simule);
        
        char score_msg[128];
        snprintf(score_msg, sizeof(score_msg), "[Système] Score aléatoire sauvé : %d/10 pour %s.", score_simule, res->nom);
        append_log(score_msg);

        free(res->parsed_text);
    } else if (res->error_msg) {
        append_log(res->error_msg);
        g_free(res->error_msg);
    } else {
        append_log("[Erreur] Impossible de lire la réponse IA.");
    }

    g_free(res->nom);
    g_free(res);

    // Arrêter l'animation et réactiver le bouton
    gtk_spinner_stop(GTK_SPINNER(spinner));
    gtk_widget_set_sensitive(btn_send, TRUE);

    return G_SOURCE_REMOVE;
}

// Le travail réseau dans un thread séparé
static gpointer thread_ask_ai(gpointer user_data) {
    AskAIData *data = (AskAIData *)user_data;
    ResultData *res = g_malloc0(sizeof(ResultData));
    res->nom = g_strdup(data->nom);

    char *raw_json = ask_ai(data->prompt_complet);
    if (raw_json != NULL) {
        res->parsed_text = parse_ai_response(raw_json);
        free(raw_json);
    } else {
        res->error_msg = g_strdup("[Erreur API] Aucune connexion ou AURA_API_KEY manquante.");
    }

    g_free(data->nom);
    g_free(data->prompt_complet);
    g_free(data);

    // Mettre à jour l'UI dans la boucle d'événements principale GTK
    g_idle_add((GSourceFunc)update_ui_after_ai, res);
    
    return NULL;
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
    append_log("> Analyse de votre réponse par l'IA Groq (Veuillez patienter...) " );
    
    AskAIData *thread_data = g_malloc(sizeof(AskAIData));
    thread_data->nom = g_strdup(nom);
    thread_data->prompt_complet = g_strdup_printf("L'étudiant a répondu : %s. Donne une note sur 10 et un court commentaire.", reponse);

    gtk_entry_set_text(GTK_ENTRY(entry_reponse), "");

    // Démarrer l'animation spinner et désactiver le bouton
    gtk_spinner_start(GTK_SPINNER(spinner));
    gtk_widget_set_sensitive(btn_send, FALSE);

    // Lancer la requête dans un thread pour ne pas freezer l'interface
    g_thread_new("AITask", thread_ask_ai, thread_data);
}

static void on_historique_clicked(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    append_log("[Système] Affichage de l'historique dans le terminal de fond.");
    show_scores();
}

int main(int argc, char *argv[]) {
    // Initialisation GTK
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Aura-CLI (Version Desktop Multi-Threaded)");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scroll, TRUE);
    text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_container_add(GTK_CONTAINER(scroll), text_view);
    gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);

    entry_nom = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_nom), "Votre nom...");
    gtk_box_pack_start(GTK_BOX(vbox), entry_nom, FALSE, FALSE, 0);

    GtkWidget *label_q = gtk_label_new("Question : Qu'est-ce qu'un pointeur en C ?");
    gtk_box_pack_start(GTK_BOX(vbox), label_q, FALSE, FALSE, 0);

    entry_reponse = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_reponse), "Votre réponse ici...");
    gtk_box_pack_start(GTK_BOX(vbox), entry_reponse, FALSE, FALSE, 0);

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    btn_send = gtk_button_new_with_label("Envoyer à l'IA");
    g_signal_connect(btn_send, "clicked", G_CALLBACK(on_envoyer_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), btn_send, TRUE, TRUE, 0);
    
    // Le nouveau composant Spinner (Indicateur de chargement anti-freeze)
    spinner = gtk_spinner_new();
    gtk_box_pack_start(GTK_BOX(hbox), spinner, FALSE, FALSE, 5);

    GtkWidget *btn_hist = gtk_button_new_with_label("Voir l'Historique");
    g_signal_connect(btn_hist, "clicked", G_CALLBACK(on_historique_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), btn_hist, TRUE, TRUE, 0);

    append_log("=== Bienvenue sur AURA Desktop ===");

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
