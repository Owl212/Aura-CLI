#ifndef API_H
#define API_H

char* demander_question(int categorie);
char* soumettre_reponse(const char* question_posee, const char* reponse_utilisateur);
char* parse_ai_response(const char* json_string);

#endif // API_H