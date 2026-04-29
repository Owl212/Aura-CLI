#ifndef DB_H
#define DB_H

void init_db();
void save_score(const char* user_name, int score, const char* categorie);
void show_scores();

#endif // DB_H