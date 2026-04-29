#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "api.h"
#include "cJSON.h"

// Callback pour l'animation d'attente (Spinner ASCII)
static int progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    // Ignorer les avertissements "paramètres non utilisés"
    (void)clientp; (void)dltotal; (void)dlnow; (void)ultotal; (void)ulnow;
    
    static int i = 0;
    const char spinner[] = "|/-\\";
    
    // '\r' renvoie le curseur au début de la ligne pour écraser le texte précédent
    printf("\r\x1b[36m> En attente de l'IA (Groq)... %c \x1b[0m", spinner[i++ % 4]);
    fflush(stdout); // Force l'affichage immédiatement
    
    return 0; // 0 = continuer le téléchargement
}

// Struct pour stocker la réponse de curl en mémoire
struct MemoryStruct {
    char *memory;
    size_t size;
};

// =====================================================================
// EXPLICATION SOUTENANCE : WriteMemoryCallback
// =====================================================================
// Cette fonction est un "callback" (une fonction de rappel) utilisée par libcurl.
// Lorsque libcurl télécharge les données de la réponse HTTP sur le réseau, 
// il ne les reçoit pas toutes d'un coup, mais par "morceaux" (chunks).
// 
// À chaque morceau reçu, libcurl appelle cette fonction.
// 
// Pointers expliqués :
// - `void *contents` : Un pointeur générique "void*" qui pointe vers les données brutes 
//   arrivées depuis le réseau.
// - `void *userp` : Un pointeur qui pointe vers notre structure de données personnalisée 
//   (`struct MemoryStruct *mem`), où l'on va accumuler la réponse.
// 
// Mémoire dynamique : 
// On utilise `realloc()` pour agrandir la taille du bloc mémoire cible afin d'y ajouter
// le nouveau texte. Ensuite, `memcpy()` copie exactement la réponse du réseau (contents) 
// à la fin du bloc mémoire agrandi (mem->memory).
// =====================================================================
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if(ptr == NULL) {
        printf("pas assez de memoire (realloc a echoue)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

// =====================================================================
// EXPLICATION SOUTENANCE : ask_ai() et libcurl
// =====================================================================
// Cette fonction centralise la configuration et l'envoi de la requête réseau.
// 
// Gestion du réseau par libcurl :
// 1. curl_easy_init() : Initialise un "handle" de session réseau (pointeur CURL *).
// 2. curl_slist : C'est une liste chaînée (utilisant des pointeurs) gérée par curl 
//    pour ajouter nos en-têtes HTTP (L'autorisation API et le Content-Type Json).
// 3. curl_easy_setopt() : Permet de configurer l'URL cible, les headers, le payload JSON,
//    et très important, de lier notre pointeur de données (`&chunk`) avec 
//    la fonction callback de lecture qu'on a défini (`WriteMemoryCallback`).
// 4. curl_easy_perform() : Bloque l'exécution temporelle et exécute la connexion 
//    avec les serveurs distants. C'est ici que l'échange réseau TCP/HTTP a lieu.
// 
// Mémoire : L'espace `chunk.memory` dynamiquement alloué doit impérativement 
// être libéré (`free()`) par l'appelant après récupération du résultat pour éviter 
// une "fuite de mémoire" (Memory leak).
// =====================================================================
char* ask_ai(const char* prompt_etudiant) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;
    
    // Initialisation
    chunk.memory = malloc(1);
    chunk.size = 0;

    // Lecture de la clé API
    const char* api_key = getenv("AURA_API_KEY");
    if (api_key == NULL) {
        fprintf(stderr, "Erreur : La variable d'environnement AURA_API_KEY n'est pas definie.\n");
        free(chunk.memory);
        return NULL;
    }

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    
    if(curl) {
        struct curl_slist *headers = NULL;
        
        // Construction des headers HTTP
        char auth_header[256];
        snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", api_key);
        headers = curl_slist_append(headers, auth_header);
        headers = curl_slist_append(headers, "Content-Type: application/json");

        // Construction du payload JSON (requête brute simple)
        // Note: On utilise llama3 comme modèle d'exemple sur Groq
        char post_data[2048];
        snprintf(post_data, sizeof(post_data),
                 "{\"model\": \"llama3-8b-8192\", \"messages\": [{\"role\": \"user\", \"content\": \"%s\"}]}",
                 prompt_etudiant);

        // Configuration de curl
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.groq.com/openai/v1/chat/completions");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
        
        // Configuration de la fonction de rappel (callback)
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        // Activation de la callback de progression (animation)
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);

        // Exécution de la requête POST
        res = curl_easy_perform(curl);
        
        // Protection 2 : Si pas d'internet ou API injoignable
        if(res != CURLE_OK) {
            fprintf(stderr, "Erreur de connexion : %s\n", curl_easy_strerror(res));
            free(chunk.memory); // On libère la mémoire pour éviter une fuite
            chunk.memory = NULL; // On retourne NULL de manière sécurisée
        }

        // Nettoyage de curl
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    
    curl_global_cleanup();
    
    // Retourne la requête brute (à free() par l'appelant plus tard !)
    return chunk.memory;
}

// =====================================================================
// EXPLICATION SOUTENANCE : parse_ai_response() et cJSON
// =====================================================================
// Cette fonction lit une chaîne de texte brute au format JSON et la transforme 
// en une arborescence d'objets manipulable via la librairie cJSON.
// 
// Pointers expliqués :
// - `const char* json_string` : Pointeur constant vers le texte brut de la réponse réseau.
// - `cJSON *json` : Un pointeur vers la racine de l'arbre représentant les hiérarchies
//   du fichier JSON en mémoire dynamique.
// 
// Navigation : On traverse cet arbre (json -> choices -> [0] -> message -> content)
// en chaînant les appels. Chaque étape retourne un pointeur vers le noeud enfant.
// Une fois le texte final trouvé, on alloue (`malloc`) un nouveau pointeur texte (`result`) 
// pour ne copier que la réponse pertinente.
//
// Mémoire : Il est impératif d'utiliser `cJSON_Delete(json)` pour détruire la racine  
// qui effacera de manière récursive tous les sous-noeuds alloués par la validation JSON !
// =====================================================================
char* parse_ai_response(const char* json_string) {
    if (json_string == NULL) return NULL;

    cJSON *json = cJSON_Parse(json_string);
    if (json == NULL) {
        fprintf(stderr, "Erreur de parsing JSON.\n");
        return NULL;
    }

    // Navigation dans le JSON : choices[0]->message->content
    cJSON *choices = cJSON_GetObjectItemCaseSensitive(json, "choices");
    cJSON *choice = cJSON_GetArrayItem(choices, 0);
    cJSON *message = cJSON_GetObjectItemCaseSensitive(choice, "message");
    cJSON *content = cJSON_GetObjectItemCaseSensitive(message, "content");

    char *result = NULL;
    
    // Si on a bien trouvé du texte, on le copie localement
    if (cJSON_IsString(content) && (content->valuestring != NULL)) {
        result = malloc(strlen(content->valuestring) + 1);
        if (result) {
            strcpy(result, content->valuestring);
        }
    } else {
        fprintf(stderr, "Impossible de trouver 'content' dans la réponse !.\n");
    }

    // Libérer proprement l'objet JSON (EVITER LES FUITES DE MEMOIRE)
    cJSON_Delete(json);
    
    return result;
}