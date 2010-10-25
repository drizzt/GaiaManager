#include "i18n.h"
#include "config.h"

int region = 1;

//Jap, English, French, Spanish, German, Italian, Dutch, Portugues, Russian, Korean, Chinese, Chinese, Finnish, Swedish, Danish, Norwegian
const char *const text_delfailed[] = {
	"Delete failed dump in",
	"Delete failed dump in",
	"Supprimer le Dump raté",
	"Borrar dump fallido en",
	"Loeschen der fehlgeschlagenen Kopie von",
	"Delete failed dump in",
	"Cancello il dump fallito in",
	"Apagar descarga falhada no",
	"Delete failed dump in",
	"Delete failed dump in",
	"Delete failed dump in",
	"Delete failed dump in",
	"Delete failed dump in",
	"Raderingen misslyckades, information i",
	"Delete failed dump in",
	"Delete failed dump in"
};

const char *const text_nosplit[] = {
	"You cannot launch split games",
	"You cannot launch split games",
	"Vous ne pouvez pas jouer à des jeux Couper",
	"No puedes jugar a juegos divididos",
	"Aufgeteilte Spiele koennen nicht gestartet werden",
	"Non puoi avviare giochi divisi",
	"You cannot launch split games",
	"Nao pode iniciar jogos divididos",
	"You cannot launch split games",
	"You cannot launch split games",
	"You cannot launch split games",
	"You cannot launch split games",
	"You cannot launch split games",
	"Du kan inte kora delade spel",
	"You cannot launch split games",
	"You cannot launch split games"
};

const char *const text_wantcopy[] = {
	"Want to copy from",
	"Want to copy from",
	"Vous-voulez le copier à partir de",
	"Quieres copiar desde",
	"Kopieren von",
	"Vuoi copiare da",
	"Want to copy from",
	"Quer copiar de",
	"Want to copy from",
	"Want to copy from",
	"Want to copy from",
	"Want to copy from",
	"Want to copy from",
	"Vill du kopiera fran",
	"Want to copy from",
	"Want to copy from"
};

const char *const text_to[] = {
	"to",
	"to",
	"vers",
	"a",
	"nach",
	"a",
	"to",
	"para",
	"to",
	"to",
	"to",
	"to",
	"to",
	"till",
	"to",
	"to"
};

const char *const text_eboot[] = {
	"EBOOT.BIN has been successfully updated",
	"EBOOT.BIN has been successfully updated",
	"EBOOT.BIN a été mis à jour avec succès",
	"EBOOT.BIN ha sido parcheado.",
	"Die EBOOT.BIN wurde erfolgreich aktualisiert",
	"EBOOT.BIN e stato aggiornato con successo",
	"EBOOT.BIN has been successfully updated",
	"EBOOT.BIN foi actualizado com sucesso",
	"EBOOT.BIN has been successfully updated",
	"EBOOT.BIN has been successfully updated",
	"EBOOT.BIN has been successfully updated",
	"EBOOT.BIN has been successfully updated",
	"EBOOT.BIN has been successfully updated",
	"Eboot.bin har uppdaterats utan problem.",
	"EBOOT.BIN has been successfully updated",
	"EBOOT.BIN has been successfully updated"
};

const char *const text_launcher[] = {
	"You can launch this utility pressing SELECT+START the next time",
	"You can launch this utility pressing SELECT+START the next time",
	"Vous pouvez lancer cet utilitaire en appuyant sur SELECT+START la prochaine fois",
	"Puedes ejecutar esta utilidad presionando SELECT+START la proxima vez.",
	"Das Spiel kann ab jetzt mit SELECT+START gestartet werden",
	"Puoi lanciare questa utility premendo SELECT+START la prossima volta",
	"You can launch this utility pressing SELECT+START the next time",
	"Pode iniciar este utilit�rio pressionando SELECT+START na proxima vez",
	"You can launch this utility pressing SELECT+START the next time",
	"You can launch this utility pressing SELECT+START the next time",
	"You can launch this utility pressing SELECT+START the next time",
	"You can launch this utility pressing SELECT+START the next time",
	"You can launch this utility pressing SELECT+START the next time",
	"Du kan nu starta detta program genom att trycka SELECT+START nasta gang.",
	"You can launch this utility pressing SELECT+START the next time",
	"You can launch this utility pressing SELECT+START the next time"
};

const char *const text_notfound[] = {
	"EBOOT.BIN not found",
	"EBOOT.BIN not found",
	"EBOOT.BIN non trouvé",
	"EBOOT.BIN no encontrado",
	"Die EBOOT.BIN wurde nicht gefunden",
	"EBOOT.BIN non trovato",
	"EBOOT.BIN not found",
	"EBOOT.BIN n�o encontrado",
	"EBOOT.BIN not found",
	"EBOOT.BIN not found",
	"EBOOT.BIN not found",
	"EBOOT.BIN not found",
	"EBOOT.BIN not found",
	"EBOOT.BIN hittades inte",
	"EBOOT.BIN not found",
	"EBOOT.BIN not found"
};

# if 0
const char *const text_wantexit[] = {
	"Want to exit?",
	"Want to exit?",
	"Voulez-vous quitter?",
	"Quieres salir?",
	"Moechten Sie die Anwendung beenden?",
	"Vuoi uscire?",
	"Want to exit?",
	"Quer sair?",
	"Want to exit?",
	"Want to exit?",
	"Want to exit?",
	"Want to exit?",
	"Want to exit?",
	"Vill du avsluta?",
	"Want to exit?",
	"Want to exit?"
};
#endif

const char *const text_wantdel[] = {
	"Want to delete from",
	"Want to delete from",
	"Voulez-vous le supprimer de",
	"Quieres borrar desde",
	"Loeschen von",
	"Vuoi cancellare da",
	"Want to delete from",
	"Quer apagar de",
	"Want to delete from",
	"Want to delete from",
	"Want to delete from",
	"Want to delete from",
	"Want to delete from",
	"Vill du radera fran",
	"Want to delete from",
	"Want to delete from"
};

const char *const text_wantuse[] = {
	"Want to use",
	"Want to use",
	"Voulez-vous utiliser",
	"Quieres usar",
	"Moechten Sie",
	"Vuoi usare",
	"Want to use",
	"Quer usar",
	"Want to use",
	"Want to use",
	"Want to use",
	"Want to use",
	"Want to use",
	"Vill du anvanda",
	"Want to use",
	"Want to use"
};

const char *const text_toinstall[] = {
	"to install the game?",
	"to install the game?",
	"pour installer le jeu?",
	"para instalar el juego?",
	"fur die Spiele-Installation verwenden?",
	"per installare il gioco?",
	"to install the game?",
	"para instalar o jogo?",
	"to install the game?",
	"to install the game?",
	"to install the game?",
	"to install the game?",
	"to install the game?",
	"till att installera spel?",
	"to install the game?",
	"to install the game?"
};

const char *const text_cover_msg[] = {
	"Do you want to download missing covers (in /dev_hdd0/" COVERS_DIR ")?\nIt could takes awhile...\nPlease wait",
	"Do you want to download missing covers (in /dev_hdd0/" COVERS_DIR ")?\nIt could takes awhile...\nPlease wait",
	"Vous-voulez telecharger les jaquettes manquantes(dans /dev_hdd0/" COVERS_DIR")?\nCela peut prendre un certain temps...\nVeuillez-Patientez",
	"Do you want to download missing covers (in /dev_hdd0/" COVERS_DIR ")?\nIt could takes awhile...\nPlease wait",
	"Moechten Sie fehlende Covers herunterladen? (nach /dev_hdd0/" COVERS_DIR ")?\nDies koennte einen Moment dauern...\nBitte warten",
	"Voi scaricare le cover mancanti(in /dev_hdd0/" COVERS_DIR ")?\nPotrebbe volerci un po'...\nAttendere prego",
	"Do you want to download missing covers (in /dev_hdd0/" COVERS_DIR ")?\nIt could takes awhile...\nPlease wait",
	"Do you want to download missing covers (in /dev_hdd0/" COVERS_DIR ")?\nIt could takes awhile...\nPlease wait",
	"Do you want to download missing covers (in /dev_hdd0/" COVERS_DIR ")?\nIt could takes awhile...\nPlease wait",
	"Do you want to download missing covers (in /dev_hdd0/" COVERS_DIR ")?\nIt could takes awhile...\nPlease wait",
	"Do you want to download missing covers (in /dev_hdd0/" COVERS_DIR ")?\nIt could takes awhile...\nPlease wait",
	"Do you want to download missing covers (in /dev_hdd0/" COVERS_DIR ")?\nIt could takes awhile...\nPlease wait",
	"Do you want to download missing covers (in /dev_hdd0/" COVERS_DIR ")?\nIt could takes awhile...\nPlease wait",
	"Do you want to download missing covers (in /dev_hdd0/" COVERS_DIR ")?\nIt could takes awhile...\nPlease wait",
	"Do you want to download missing covers (in /dev_hdd0/" COVERS_DIR ")?\nIt could takes awhile...\nPlease wait",
	"Do you want to download missing covers (in /dev_hdd0/" COVERS_DIR ")?\nIt could takes awhile...\nPlease wait"
};

// OSD Graphics - only use ASCII characters

const char *const text_launch[] = {
	"Launch Selected",
	"Launch Selected",
	"Lancer Selection",
	"Launch Selected",
	"Auswahl starten",
	"Lancia Selezione",
	"Launch Selected",
	"Launch Selected",
	"Launch Selected",
	"Launch Selected",
	"Launch Selected",
	"Launch Selected",
	"Launch Selected",
	"Launch Selected",
	"Launch Selected",
	"Launch Selected"
};

const char *const text_download[] = {
	"Download Covers",
	"Download Covers",
	"Telechar Jaquettes",
	"Download Covers",
	"Covers herunterladen",
	"Scarica Cover",
	"Download Covers",
	"Download Covers",
	"Download Covers",
	"Download Covers",
	"Download Covers",
	"Download Covers",
	"Download Covers",
	"Download Covers",
	"Download Covers",
	"Download Covers"
};

const char *const text_copy[] = {
	"Copy Selected",
	"Copy Selected",
	"Copier Selection",
	"Copy Selected",
	"Auswahl kopieren",
	"Copia Selezione",
	"Copy Selected",
	"Copy Selected",
	"Copy Selected",
	"Copy Selected",
	"Copy Selected",
	"Copy Selected",
	"Copy Selected",
	"Copy Selected",
	"Copy Selected",
	"Copy Selected"
};

const char *const text_delete[] = {
	"Delete Selected",
	"Delete Selected",
	"Effacer Selection",
	"Delete Selected",
	"Auswahl loeschen",
	"Cancella Selezione",
	"Delete Selected",
	"Delete Selected",
	"Delete Selected",
	"Delete Selected",
	"Delete Selected",
	"Delete Selected",
	"Delete Selected",
	"Delete Selected",
	"Delete Selected",
	"Delete Selected"
};

const char *const text_refresh[] = {
	"Refresh List",
	"Refresh List",
	"Rafraichir Liste",
	"Refresh List",
	"Liste neuladen",
	"Aggiorna Lista",
	"Refresh List",
	"Refresh List",
	"Refresh List",
	"Refresh List",
	"Refresh List",
	"Refresh List",
	"Refresh List",
	"Refresh List",
	"Refresh List",
	"Refresh List"
};

const char *const text_check[] = {
	"Check Selected",
	"Check Selected",
	"Verifier Selection",
	"Check Selected",
	"Auswahl ueberpruefen",
	"Verifica Selezione",
	"Check Selected",
	"Check Selected",
	"Check Selected",
	"Check Selected",
	"Check Selected",
	"Check Selected",
	"Check Selected",
	"Check Selected",
	"Check Selected",
	"Check Selected"
};

const char *const text_discless[] = {
	"Disc Less",
	"Disc Less",
	"Sans Disque",
	"Disc Less",
	"Ohne Disk",
	"Senza Disco",
	"Disc Less",
	"Disc Less",
	"Disc Less",
	"Disc Less",
	"Disc Less",
	"Disc Less",
	"Disc Less",
	"Disc Less",
	"Disc Less",
	"Disc Less"
};

const char *const text_mempatch[] = { 
	"Mem Patch",
	"Mem Patch",
	"Mem Patch",
	"Mem Patch",
	"Mem Patch",
	"Mem Patch",
	"Mem Patch",
	"Mem Patch",
	"Mem Patch",
	"Mem Patch",
	"Mem Patch",
	"Mem Patch",
	"Mem Patch",
	"Mem Patch",
	"Mem Patch",
	"Mem Patch"
};

const char *const text_directboot[] = {
	"Direct Boot",
	"Direct Boot",
	"Direct Boot",
	"Direct Boot",
	"Direkt Boot",
	"Boot Diretto",
	"Direct Boot",
	"Direct Boot",
	"Direct Boot",
	"Direct Boot",
	"Direct Boot",
	"Direct Boot",
	"Direct Boot",
	"Direct Boot",
	"Direct Boot",
	"Direct Boot"
};

const char *const text_ftpserver[] = {
	"FTP Server",
	"FTP Server",
	"FTP Serveur",
	"FTP Server",
	"FTP Server",
	"FTP Server",
	"FTP Server",
	"FTP Server",
	"FTP Server",
	"FTP Server",
	"FTP Server",
	"FTP Server",
	"FTP Server",
	"FTP Server",
	"FTP Server",
	"FTP Server"
};

const char *const text_ommode[] = {
	"OM Mode",
	"OM Mode",
	"OM Mode",
	"OM Mode",
	"OM Modus:",
	"OM Mode",
	"OM Mode",
	"OM Mode",
	"OM Mode",
	"OM Mode",
	"OM Mode",
	"OM Mode",
	"OM Mode",
	"OM Mode",
	"OM Mode",
	"OM Mode"
};

const char *const text_homebrew[] = {
	"HOMEBREW",
	"HOMEBREW",
	"HOMEBREW",
	"HOMEBREW",
	"Homebrew",
	"HOMEBREW",
	"HOMEBREW",
	"HOMEBREW",
	"HOMEBREW",
	"HOMEBREW",
	"HOMEBREW",
	"HOMEBREW",
	"HOMEBREW",
	"HOMEBREW",
	"HOMEBREW",
	"HOMEBREW"
};

const char *const text_game[] = {
	"GAME",
	"GAME",
	"JEUX",
	"GAME",
	"Spiel",
	"GIOCO",
	"GAME",
	"GAME",
	"GAME",
	"GAME",
	"GAME",
	"GAME",
	"GAME",
	"GAME",
	"GAME",
	"GAME"
};

/* vim: set ts=4 sw=4 sts=4 tw=120 */
