#pragma once

#include <string>

enum class Log_Level {
    VERBOSE,
    INFO,
    WARNING,
    ERROR,
    INVALID
};

struct Logger {
    Log_Level   level = Log_Level::WARNING;
};

void        set_log_level(Logger* logger, Log_Level level);
Log_Level   get_log_level(Logger* logger);
void        log(Logger* logger, Log_Level level, const char *format, ...);
void        flush_logs(Logger* logger);
void        open_log_console(); /// Open a console under Windows prepared to receive logs

// @TODO Ajouter une méthode pour ajouter un logger custom pour printer dans la console
// de la GUI par exemple. (avec un pointeur sur fontion)
// Ajouter le support d'enregistrement dans un fichier, mais dans ce cas regarder
// les dates pour supprimer les anciens si le poids total est dépassé.
// Changer le niveau des logs dans les fichiers et l'output pour la version release non dev.
