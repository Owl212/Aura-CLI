#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "api.h"
#include "db.h"

// Pointeurs globaux
GtkWidget *text_view;
GtkWidget *entry_nom;
GtkTextBuffer *buffer;

// Contexte pour chaque onglet
typedef struct {
    const gchar *category;
    GtkWidget *label_question;
    GtkWidget *entry_reponse;
    GtkWidget *spinner;
    GtkWidget *btn_gen;
    GtkWidget *btn_submit;
} TabContext;

TabContext tabs[3];

// Types pour le threading
typedef struct {
    int action; // 0 = generate, 1 = evaluate
    TabContext *tab;
    gchar *prompt;
    gchar *nom;
} ThreadData;

typedef struct {
    int action;
    TabContext *tab;
    gchar *parsed_text;
    gchar *error_msg;
    gchar *nom;
} ThreadResult;

// Logger
void append_log(const gchar *text) {
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(buffer, &iter);
    gtk_text_buffer_insert(buffer, &iter, text, -1);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    
    GtkTextMark *mark = gtk_text_buffer_create_mark(buffer, NULL, &iter, FALSE);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(text_view), mark, 0.0, FALSE, 0.0, 0.0);
    gtk_text_buffer_delete_mark(buffer, mark);
}

// Maj UI après thread
static gboolean update_ui_after_ai(gpointer user_data) {
    ThreadResult *res = (ThreadResult *)user_data;
    TabContext *tab = res->tab;

    gtk_spinner_stop(GTK_SPINNER(tab->spinner));
    gtk_widget_set_sensitive(tab->btn_gen, TRUE);
    gtk_widget_set_sensitive(tab->btn_submit, TRUE);

    if (res->parsed_text) {
        if (res->action == 0) {
            // Génération de question
            gtk_label_set_text(GTK_LABEL(tab->label_question), res->parsed_text);
            append_log("[Système] Nouvelle question générée avec succès.");
        } else {
            // Évaluation de réponse
            append_log("[IA Aura] Évaluation :");
            append_log(res->parsed_text);
            
            int score_simule = rand() % 11;
            save_score(res->nom, score_simule);
            
            char score_msg[128];
            snprintf(score_msg, sizeof(score_msg), "[Système] Score de %d/10 sauvegardé en BDD pour %s.", score_simule, res->nom);
            append_log(score_msg);
        }
        free(res->parsed_text);
    } else if (res->error_msg) {
        append_log(res->error_msg);
        g_free(res->error_msg);
    } else {
        append_log("[Erreur] Réponse indisponible.");
    }

    if (res->nom) g_free(res->nom);
    g_free(res);
    return G_SOURCE_REMOVE;
}

// Thread réseau
static gpointer thread_ask_ai(gpointer user_data) {
    ThreadData *data = (ThreadData *)user_data;
    ThreadResult *res = g_malloc0(sizeof(ThreadResult));
    res->action = data->action;
    res->tab = data->tab;
    if (data->nom) res->nom = g_strdup(data->nom);

    char *raw_json = ask_ai(data->prompt);
    if (raw_json != NULL) {
        res->parsed_text = parse_ai_response(raw_json);
        free(raw_json);
    } else {
        res->error_msg = g_strdup("[Erreur API] Aucune connexion ou API Key invalide.");
    }

    g_free(data->prompt);
    if (data->nom) g_free(data->nom);
    g_free(data);

    g_idle_add((GSourceFunc)update_ui_after_ai, res);
    return NULL;
}

// Callbacks
static void on_generate_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    TabContext *tab = (TabContext *)data;
    
    append_log("> IA sollicitée pour générer une question...");
    gtk_label_set_text(GTK_LABEL(tab->label_question), "Génération en cours...");
    
    gtk_spinner_start(GTK_SPINNER(tab->spinner));
    gtk_widget_set_sensitive(tab->btn_gen, FALSE);
    gtk_widget_set_sensitive(tab->btn_submit, FALSE);

    ThreadData *td = g_malloc0(sizeof(ThreadData));
    td->action = 0;
    td->tab = tab;
    td->prompt = g_strdup_printf("Tu es un examinateur. Pose une seule question courte sur le thème '%s'. Ne donne pas la réponse. Format brut.", tab->category);

    g_thread_new("AIGen", thread_ask_ai, td);
}

static void on_submit_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    TabContext *tab = (TabContext *)data;
    
    const gchar *nom = gtk_entry_get_text(GTK_ENTRY(entry_nom));
    const gchar *question = gtk_label_get_text(GTK_LABEL(tab->label_question));
    const gchar *reponse = gtk_entry_get_text(GTK_ENTRY(tab->entry_reponse));

    if (strlen(nom) == 0 || strlen(reponse) == 0) {
        append_log("[Système] Veuillez renseigner votre nom en haut, et votre réponse.");
        return;
    }

    char log_msg[512];
    snprintf(log_msg, sizeof(log_msg), "[%s] Réponse : %s", nom, reponse);
    append_log(log_msg);
    append_log("> Évaluation de la réponse en cours...");

    gtk_entry_set_text(GTK_ENTRY(tab->entry_reponse), "");
    gtk_spinner_start(GTK_SPINNER(tab->spinner));
    gtk_widget_set_sensitive(tab->btn_gen, FALSE);
    gtk_widget_set_sensitive(tab->btn_submit, FALSE);

    ThreadData *td = g_malloc0(sizeof(ThreadData));
    td->action = 1;
    td->tab = tab;
    td->nom = g_strdup(nom);
    td->prompt = g_strdup_printf("A la question '%s', l'étudiant a répondu '%s'. Donne une note sur 10 et un commentaire court. Format brut.", question, reponse);

    g_thread_new("AIEval", thread_ask_ai, td);
}

static void on_historique_clicked(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    append_log("[Système] L'historique s'affiche dans la console MSYS2.");
    show_scores();
}

int main(int argc, char *argv[]) {
    if (getenv("AURA_API_KEY") == NULL) {
        printf("Clé API manquante dans l'environnement\n");
        return 1;
    }

    init_db();
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Aura-CLI - Multi-Thèmes");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // HEADER : Nom et Historique
    GtkWidget *hbox_header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(vbox), hbox_header, FALSE, FALSE, 0);

    GtkWidget *label_nom = gtk_label_new("Votre Nom:");
    gtk_box_pack_start(GTK_BOX(hbox_header), label_nom, FALSE, FALSE, 0);

    entry_nom = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_nom), "Prénom Nom");
    gtk_box_pack_start(GTK_BOX(hbox_header), entry_nom, TRUE, TRUE, 0);

    GtkWidget *btn_hist = gtk_button_new_with_label("Voir Historique Global");
    g_signal_connect(btn_hist, "clicked", G_CALLBACK(on_historique_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(hbox_header), btn_hist, FALSE, FALSE, 0);

    // LE NOTEBOOK (ONGLETS)
    GtkWidget *notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(vbox), notebook, FALSE, FALSE, 10);

    const char* categories[] = {"Algorithmique", "Énigmes Mathématiques", "Culture Générale IT"};
    for (int i = 0; i < 3; i++) {
        tabs[i].category = categories[i];

        GtkWidget *page_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_container_set_border_width(GTK_CONTAINER(page_vbox), 10);

        tabs[i].btn_gen = gtk_button_new_with_label("1. Générer une question");
        gtk_box_pack_start(GTK_BOX(page_vbox), tabs[i].btn_gen, FALSE, FALSE, 0);

        tabs[i].label_question = gtk_label_new("Appuyez sur 'Générer' pour recevoir une question...");
        gtk_label_set_line_wrap(GTK_LABEL(tabs[i].label_question), TRUE);
        gtk_widget_set_margin_top(tabs[i].label_question, 20);
        gtk_widget_set_margin_bottom(tabs[i].label_question, 20);
        // Important: alignement à gauche de la question
        gtk_widget_set_halign(tabs[i].label_question, GTK_ALIGN_START);
        gtk_box_pack_start(GTK_BOX(page_vbox), tabs[i].label_question, FALSE, FALSE, 0);

        tabs[i].entry_reponse = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(tabs[i].entry_reponse), "Votre réponse à l'IA...");
        gtk_box_pack_start(GTK_BOX(page_vbox), tabs[i].entry_reponse, FALSE, FALSE, 0);

        GtkWidget *hbox_acc = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        tabs[i].btn_submit = gtk_button_new_with_label("2. Soumettre la réponse");
        tabs[i].spinner = gtk_spinner_new();
        gtk_box_pack_start(GTK_BOX(hbox_acc), tabs[i].btn_submit, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(hbox_acc), tabs[i].spinner, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(page_vbox), hbox_acc, FALSE, FALSE, 0);

        g_signal_connect(tabs[i].btn_gen, "clicked", G_CALLBACK(on_generate_clicked), &tabs[i]);
        g_signal_connect(tabs[i].btn_submit, "clicked", G_CALLBACK(on_submit_clicked), &tabs[i]);

        GtkWidget *tab_label = gtk_label_new(categories[i]);
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_vbox, tab_label);
    }

    // LOGS (TEXT VIEW) ZONE
    GtkWidget *label_log = gtk_label_new("Journal d'activité :");
    gtk_widget_set_halign(label_log, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox), label_log, FALSE, FALSE, 0);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scroll, TRUE);
    text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_container_add(GTK_CONTAINER(scroll), text_view);
    gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);

    append_log("=== Bienvenue sur AURA Desktop (Multi-Thèmes) ===");

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}