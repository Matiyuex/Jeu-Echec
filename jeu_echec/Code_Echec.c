#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>
#include <time.h>

// Structure pour suivre l'état du jeu (roque et prise en passant)
typedef struct {
    int roiBlancBouge, tourBlancheA1Bouge, tourBlancheH1Bouge;
    int roiNoirBouge, tourNoireA8Bouge, tourNoireH8Bouge;
    int enPassantX, enPassantY;
    int dernierTourDouble;
} GameState;

// Structure pour stocker un mouvement
typedef struct {
    char start[3];
    char end[3];
} Mouvement;

// Structure pour sauvegarder une partie complète
typedef struct {
    char plateau[8][8];
    GameState state;
    int tourBlancs;
    int scoreBlancs;
    int scoreNoirs;
    int tourActuel;
    const char *description; // Description du scénario
} SavedGame;

// Prototypes des fonctions
void convertirPosition(const char *pos, int *x, int *y);
int echec(char PLATEAU[8][8], int joueur, GameState *state);
int estEchecEtMat(char PLATEAU[8][8], int joueur, GameState *state);
int estPat(char PLATEAU[8][8], int joueur, GameState *state);
int deplacerPiece(char PLATEAU[8][8], const char *start, const char *end, int verifierEchec, int silence, GameState *state);
void iniTableau(char PLATEAU[8][8], int Lar, int Long, GameState *state);
void afficherTableau(char PLATEAU[8][8], int Lar, int Long);
int validerEntree(const char *pos);
int pion(char PLATEAU[8][8], const char *start, const char *end, int silence, GameState *state);
int fou(char PLATEAU[8][8], const char *start, const char *end, int silence);
int tour(char PLATEAU[8][8], const char *start, const char *end, int silence);
int cavalier(char PLATEAU[8][8], const char *start, const char *end, int silence);
int Queen(char PLATEAU[8][8], const char *start, const char *end, int silence);
int King(char PLATEAU[8][8], const char *start, const char *end, int silence, GameState *state);
void partie(char PLATEAU[8][8], int Lar, int Long, GameState *state, int tourBlancs, int scoreBlancs, int scoreNoirs, int tourActuel);
char demanderPromotion(int joueur);
int estMatchNulMateriel(char PLATEAU[8][8]);
void afficherMenu();
void jouerContreJoueur();
void jouerContreIA();
void afficherRegles();
void chargerPartieSauvegardee();
void initialiserPartiePriseEnPassant(SavedGame *game);
void initialiserPartieRoque(SavedGame *game);
void initialiserPartiePromotion(SavedGame *game);
void initialiserPartieEchecEtMat(SavedGame *game);
void initialiserPartiePat(SavedGame *game);
int trouverMouvementsLegaux(char PLATEAU[8][8], int joueur, GameState *state, Mouvement *mouvements, int maxMouvements);
int choisirMouvementAleatoire(Mouvement *mouvements, int nombreMouvements, Mouvement *mouvementChoisi);

// Initialisation du plateau par défaut
void iniTableau(char PLATEAU[8][8], int Lar, int Long, GameState *state) {
    for (int i = 0; i < Lar; i++) {
        for (int j = 0; j < Long; j++) {
            PLATEAU[i][j] = ' ';
        }
    }
    // Configuration standard d'une partie d'échecs
    PLATEAU[0][0] = 't'; PLATEAU[0][1] = 'c'; PLATEAU[0][2] = 'f'; PLATEAU[0][3] = 'q';
    PLATEAU[0][4] = 'k'; PLATEAU[0][5] = 'f'; PLATEAU[0][6] = 'c'; PLATEAU[0][7] = 't';
    for (int j = 0; j < 8; j++) PLATEAU[1][j] = 'p';
    PLATEAU[7][0] = 'T'; PLATEAU[7][1] = 'C'; PLATEAU[7][2] = 'F'; PLATEAU[7][3] = 'Q';
    PLATEAU[7][4] = 'K'; PLATEAU[7][5] = 'F'; PLATEAU[7][6] = 'C'; PLATEAU[7][7] = 'T';
    for (int j = 0; j < 8; j++) PLATEAU[6][j] = 'P';
    state->roiBlancBouge = 0;
    state->tourBlancheA1Bouge = 0;
    state->tourBlancheH1Bouge = 0;
    state->roiNoirBouge = 0;
    state->tourNoireA8Bouge = 0;
    state->tourNoireH8Bouge = 0;
    state->enPassantX = -1;
    state->enPassantY = -1;
    state->dernierTourDouble = -1;
}

// Affichage du plateau
void afficherTableau(char PLATEAU[8][8], int Lar, int Long) {
    printf("\n   a  b  c  d  e  f  g  h\n");
    printf("-------------------------\n");
    for (int i = 0; i < Lar; i++) {
        printf("%d ", 8 - i);
        for (int j = 0; j < Long; j++) {
            printf(" %c ", (PLATEAU[i][j] == ' ') ? '.' : PLATEAU[i][j]);
        }
        printf(" | %d\n", 8 - i);
    }
    printf("-------------------------\n");
    printf("   a  b  c  d  e  f  g  h\n");
}

// Validation des entrées utilisateur
int validerEntree(const char *position) {
    if (strlen(position) != 2) return 0;
    if (position[0] < 'a' || position[0] > 'h') return 0;
    if (position[1] < '1' || position[1] > '8') return 0;
    return 1;
}

// Conversion d'une position style "e2" en indices de tableau
void convertirPosition(const char *position, int *x, int *y) {
    *y = position[0] - 'a';
    *x = 8 - (position[1] - '0');
}

// Demander la promotion du pion
char demanderPromotion(int joueur) {
    char choix;
    while (1) {
        printf("Choisissez la pièce pour la promotion (q: reine, t: tour, f: fou, c: cavalier) : ");
        scanf(" %c", &choix);
        choix = tolower(choix);
        if (choix == 'q' || choix == 't' || choix == 'f' || choix == 'c') {
            return (joueur == 2) ? toupper(choix) : choix;
        }
        printf("Choix invalide. Veuillez choisir q, t, f ou c.\n");
    }
}

// Fonctions de mouvement des pièces
int pion(char PLATEAU[8][8], const char *start, const char *end, int silence, GameState *state) {
    int x1, y1, x2, y2;
    convertirPosition(start, &x1, &y1);
    convertirPosition(end, &x2, &y2);

    char piece = PLATEAU[x1][y1];
    if (piece == 'P') {
        if (x2 == x1 - 1 && y2 == y1 && PLATEAU[x2][y2] == ' ') return 1;
        if (x1 == 6 && x2 == x1 - 2 && y2 == y1 && PLATEAU[x1 - 1][y1] == ' ' && PLATEAU[x2][y2] == ' ') return 1;
        if (x2 == x1 - 1 && abs(y2 - y1) == 1 && PLATEAU[x2][y2] >= 'a' && PLATEAU[x2][y2] <= 'z') return 1;
        if (state->enPassantX >= 0 && state->enPassantY >= 0) {
            if (x2 == state->enPassantX && y2 == state->enPassantY && abs(y2 - y1) == 1 && PLATEAU[x2][y2] == ' ' && PLATEAU[x1][y2] >= 'a' && PLATEAU[x1][y2] <= 'z') {
                return 1;
            }
        }
    } else if (piece == 'p') {
        if (x2 == x1 + 1 && y2 == y1 && PLATEAU[x2][y2] == ' ') return 1;
        if (x1 == 1 && x2 == x1 + 2 && y2 == y1 && PLATEAU[x1 + 1][y1] == ' ' && PLATEAU[x2][y2] == ' ') return 1;
        if (x2 == x1 + 1 && abs(y2 - y1) == 1 && PLATEAU[x2][y2] >= 'A' && PLATEAU[x2][y2] <= 'Z') return 1;
        if (state->enPassantX >= 0 && state->enPassantY >= 0) {
            if (x2 == state->enPassantX && y2 == state->enPassantY && abs(y2 - y1) == 1 && PLATEAU[x2][y2] == ' ' && PLATEAU[x1][y2] >= 'A' && PLATEAU[x1][y2] <= 'Z') {
                return 1;
            }
        }
    }
    return 0;
}

int fou(char PLATEAU[8][8], const char *start, const char *end, int silence) {
    int x1, y1, x2, y2;
    convertirPosition(start, &x1, &y1);
    convertirPosition(end, &x2, &y2);

    if (abs(x2 - x1) != abs(y2 - y1)) return 0;

    int stepX = (x2 > x1) ? 1 : -1;
    int stepY = (y2 > y1) ? 1 : -1;
    int i = x1 + stepX, j = y1 + stepY;
    while (i != x2 && j != y2) {
        if (PLATEAU[i][j] != ' ') return 0;
        i += stepX;
        j += stepY;
    }

    char piece = PLATEAU[x1][y1];
    char target = PLATEAU[x2][y2];
    if (target == ' ') return 1;
    if ((piece == 'F' && target >= 'a' && target <= 'z') || (piece == 'f' && target >= 'A' && target <= 'Z')) return 1;
    return 0;
}

int tour(char PLATEAU[8][8], const char *start, const char *end, int silence) {
    int x1, y1, x2, y2;
    convertirPosition(start, &x1, &y1);
    convertirPosition(end, &x2, &y2);
    if (x1 != x2 && y1 != y2) {
        return 0;
    }

    int stepX = (x2 == x1) ? 0 : (x2 > x1 ? 1 : -1);
    int stepY = (y2 == y1) ? 0 : (y2 > y1 ? 1 : -1);
    int i = x1 + stepX, j = y1 + stepY;
    while (i != x2 || j != y2) {
        if (PLATEAU[i][j] != ' ') {
            return 0;
        }
        i += stepX;
        j += stepY;
    }

    char piece = PLATEAU[x1][y1];
    char target = PLATEAU[x2][y2];
    if (target == ' ') return 1;
    if ((piece == 'T' && target >= 'a' && target <= 'z') || (piece == 't' && target >= 'A' && target <= 'Z')) return 1;
    return 0;
}

int cavalier(char PLATEAU[8][8], const char *start, const char *end, int silence) {
    int x1, y1, x2, y2;
    convertirPosition(start, &x1, &y1);
    convertirPosition(end, &x2, &y2);

    int dx = abs(x2 - x1), dy = abs(y2 - y1);
    if (!((dx == 2 && dy == 1) || (dx == 1 && dy == 2))) return 0;

    char piece = PLATEAU[x1][y1];
    char target = PLATEAU[x2][y2];
    if (target == ' ') return 1;
    if ((piece == 'C' && target >= 'a' && target <= 'z') || (piece == 'c' && target >= 'A' && target <= 'Z')) return 1;
    return 0;
}

int Queen(char PLATEAU[8][8], const char *start, const char *end, int silence) {
    int x1, y1, x2, y2;
    convertirPosition(start, &x1, &y1);
    convertirPosition(end, &x2, &y2);

    int dx = x2 - x1;
    int dy = y2 - y1;
    int isDiagonal = (abs(dx) == abs(dy));
    int isLineOrColumn = (dx == 0 || dy == 0);

    if (!isDiagonal && !isLineOrColumn) {
        return 0;
    }

    int stepX, stepY;
    if (isDiagonal) {
        stepX = (dx > 0) ? 1 : -1;
        stepY = (dy > 0) ? 1 : -1;
    } else if (isLineOrColumn) {
        stepX = (dx == 0) ? 0 : (dx > 0 ? 1 : -1);
        stepY = (dy == 0) ? 0 : (dy > 0 ? 1 : -1);
    } else {
        return 0;
    }

    int i = x1 + stepX, j = y1 + stepY;
    while (i != x2 || j != y2) {
        if (PLATEAU[i][j] != ' ') {
            return 0;
        }
        i += stepX;
        j += stepY;
    }

    char piece = PLATEAU[x1][y1];
    char target = PLATEAU[x2][y2];
    if (target == ' ') {
        return 1;
    }
    if ((piece == 'Q' && target >= 'a' && target <= 'z') || (piece == 'q' && target >= 'A' && target <= 'Z')) {
        return 1;
    }

    return 0;
}

int King(char PLATEAU[8][8], const char *start, const char *end, int silence, GameState *state) {
    int x1, y1, x2, y2;
    convertirPosition(start, &x1, &y1);
    convertirPosition(end, &x2, &y2);

    char piece = PLATEAU[x1][y1];
    int joueur = (piece == 'K') ? 2 : 1;

    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    if (dx <= 1 && dy <= 1 && !(dx == 0 && dy == 0)) {
        char target = PLATEAU[x2][y2];
        if (target == ' ' || 
            (piece == 'K' && target >= 'a' && target <= 'z') ||
            (piece == 'k' && target >= 'A' && target <= 'Z')) {
            return 1;
        }
        return 0;
    }

    int isPetitRoque = 0, isGrandRoque = 0;
    if (piece == 'K') {
        if (x1 == 7 && y1 == 4 && x2 == 7 && y2 == 6) {
            isPetitRoque = 1;
        } else if (x1 == 7 && y1 == 4 && x2 == 7 && y2 == 2) {
            isGrandRoque = 1;
        }
    } else if (piece == 'k') {
        if (x1 == 0 && y1 == 4 && x2 == 0 && y2 == 6) {
            isPetitRoque = 1;
        } else if (x1 == 0 && y1 == 4 && x2 == 0 && y2 == 2) {
            isGrandRoque = 1;
        }
    }

    if (isPetitRoque || isGrandRoque) {
        if (piece == 'K') {
            if (state->roiBlancBouge) return 0;
            if (isPetitRoque && state->tourBlancheH1Bouge) return 0;
            if (isGrandRoque && state->tourBlancheA1Bouge) return 0;
        } else {
            if (state->roiNoirBouge) return 0;
            if (isPetitRoque && state->tourNoireH8Bouge) return 0;
            if (isGrandRoque && state->tourNoireA8Bouge) return 0;
        }

        if (isPetitRoque) {
            int row = (piece == 'K') ? 7 : 0;
            if (PLATEAU[row][5] != ' ' || PLATEAU[row][6] != ' ') return 0;
        } else if (isGrandRoque) {
            int row = (piece == 'K') ? 7 : 0;
            if (PLATEAU[row][1] != ' ' || PLATEAU[row][2] != ' ' || PLATEAU[row][3] != ' ') return 0;
        }

        char copie[8][8];
        memcpy(copie, PLATEAU, sizeof(char) * 64);
        GameState stateCopy = *state;
        if (echec(copie, joueur, &stateCopy)) return 0;

        if (isPetitRoque) {
            memcpy(copie, PLATEAU, sizeof(char) * 64);
            int row = (piece == 'K') ? 7 : 0;
            copie[row][4] = ' ';
            copie[row][5] = piece;
            if (echec(copie, joueur, &stateCopy)) return 0;
        } else if (isGrandRoque) {
            memcpy(copie, PLATEAU, sizeof(char) * 64);
            int row = (piece == 'K') ? 7 : 0;
            copie[row][4] = ' ';
            copie[row][3] = piece;
            if (echec(copie, joueur, &stateCopy)) return 0;
        }

        return 1;
    }

    return 0;
}

// Vérification de match nul par matériel
int estMatchNulMateriel(char PLATEAU[8][8]) {
    int piecesBlanches = 0, piecesNoires = 0;
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            char piece = PLATEAU[i][j];
            if (piece >= 'A' && piece <= 'Z' && piece != 'K') piecesBlanches++;
            if (piece >= 'a' && piece <= 'z' && piece != 'k') piecesNoires++;
        }
    }
    if (piecesBlanches == 0 && piecesNoires == 0) return 1;
    return 0;
}

// Déplacement de pièce
int deplacerPiece(char PLATEAU[8][8], const char *start, const char *end, int verifierEchec, int silence, GameState *state) {
    int x1, y1, x2, y2;
    convertirPosition(start, &x1, &y1);
    convertirPosition(end, &x2, &y2);
    char piece = PLATEAU[x1][y1];

    if (x1 < 0 || x1 >= 8 || y1 < 0 || y1 >= 8 || x2 < 0 || x2 >= 8 || y2 < 0 || y2 >= 8) {
        return 0;
    }

    if (piece == ' ') {
        return 0;
    }

    if ((piece >= 'a' && piece <= 'z' && PLATEAU[x2][y2] >= 'a' && PLATEAU[x2][y2] <= 'z') ||
        (piece >= 'A' && piece <= 'Z' && PLATEAU[x2][y2] >= 'A' && PLATEAU[x2][y2] <= 'Z')) {
        return 0;
    }

    int mouvementValide = 0;
    if (piece == 'p' || piece == 'P') mouvementValide = pion(PLATEAU, start, end, silence, state);
    else if (piece == 'f' || piece == 'F') mouvementValide = fou(PLATEAU, start, end, silence);
    else if (piece == 't' || piece == 'T') mouvementValide = tour(PLATEAU, start, end, silence);
    else if (piece == 'c' || piece == 'C') mouvementValide = cavalier(PLATEAU, start, end, silence);
    else if (piece == 'q' || piece == 'Q') mouvementValide = Queen(PLATEAU, start, end, silence);
    else if (piece == 'k' || piece == 'K') mouvementValide = King(PLATEAU, start, end, silence, state);
    else {
        return 0;
    }

    if (!mouvementValide) {
        return 0;
    }

    int enPassantXAvant = state->enPassantX;
    int enPassantYAvant = state->enPassantY;
    state->enPassantX = -1;
    state->enPassantY = -1;

    int isPetitRoque = 0, isGrandRoque = 0;
    if (piece == 'K') {
        if (x1 == 7 && y1 == 4 && x2 == 7 && y2 == 6) isPetitRoque = 1;
        else if (x1 == 7 && y1 == 4 && x2 == 7 && y2 == 2) isGrandRoque = 1;
    } else if (piece == 'k') {
        if (x1 == 0 && y1 == 4 && x2 == 0 && y2 == 6) isPetitRoque = 1;
        else if (x1 == 0 && y1 == 4 && x2 == 0 && y2 == 2) isGrandRoque = 1;
    }

    if (isPetitRoque) {
        int row = (piece == 'K') ? 7 : 0;
        PLATEAU[row][4] = ' ';
        PLATEAU[row][6] = piece;
        PLATEAU[row][7] = ' ';
        PLATEAU[row][5] = (piece == 'K') ? 'T' : 't';
    } else if (isGrandRoque) {
        int row = (piece == 'K') ? 7 : 0;
        PLATEAU[row][4] = ' ';
        PLATEAU[row][2] = piece;
        PLATEAU[row][0] = ' ';
        PLATEAU[row][3] = (piece == 'K') ? 'T' : 't';
    } else {
        char pieceCapturee = PLATEAU[x2][y2];
        char nouvellePiece = piece;
        if (!silence && ((piece == 'P' && x2 == 0 && x1 != 0) || (piece == 'p' && x2 == 7 && x1 != 7))) {
            nouvellePiece = demanderPromotion((piece == 'P') ? 2 : 1);
        }
        if (piece == 'P' && x2 == enPassantXAvant && y2 == enPassantYAvant && abs(y2 - y1) == 1 && PLATEAU[x2][y2] == ' ' && PLATEAU[x1][y2] >= 'a' && PLATEAU[x1][y2] <= 'z') {
            PLATEAU[x1][y2] = ' ';
        } else if (piece == 'p' && x2 == enPassantXAvant && y2 == enPassantYAvant && abs(y2 - y1) == 1 && PLATEAU[x2][y2] == ' ' && PLATEAU[x1][y2] >= 'A' && PLATEAU[x1][y2] <= 'Z') {
            PLATEAU[x1][y2] = ' ';
        }
        PLATEAU[x2][y2] = nouvellePiece;
        PLATEAU[x1][y1] = ' ';
        int joueur = (piece >= 'A' && piece <= 'Z') ? 2 : 1;
        if (verifierEchec && echec(PLATEAU, joueur, state)) {
            PLATEAU[x1][y1] = piece;
            PLATEAU[x2][y2] = pieceCapturee;
            state->enPassantX = enPassantXAvant;
            state->enPassantY = enPassantYAvant;
            return 0;
        }
        if (piece == 'P' && x1 == 6 && x2 == 4 && y1 == y2) {
            state->enPassantX = 5;
            state->enPassantY = y2;
            state->dernierTourDouble = state->dernierTourDouble + 1;
        } else if (piece == 'p' && x1 == 1 && x2 == 3 && y1 == y2) {
            state->enPassantX = 4;
            state->enPassantY = y2;
            state->dernierTourDouble = state->dernierTourDouble + 1;
        }
    }

    if (piece == 'K') state->roiBlancBouge = 1;
    else if (piece == 'k') state->roiNoirBouge = 1;
    else if (piece == 'T') {
        if (x1 == 7 && y1 == 0) state->tourBlancheA1Bouge = 1;
        else if (x1 == 7 && y1 == 7) state->tourBlancheH1Bouge = 1;
    } else if (piece == 't') {
        if (x1 == 0 && y1 == 0) state->tourNoireA8Bouge = 1;
        else if (x1 == 0 && y1 == 7) state->tourNoireH8Bouge = 1;
    }

    return 1;
}

// Vérification d'échec
int echec(char PLATEAU[8][8], int joueur, GameState *state) {
    int roiX = -1, roiY = -1;
    char roi = (joueur == 1) ? 'k' : 'K';

    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            if (PLATEAU[x][y] == roi) {
                roiX = x;
                roiY = y;
                break;
            }
        }
    }

    if (roiX == -1 || roiY == -1) {
        return 0;
    }

    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            char piece = PLATEAU[x][y];
            if ((joueur == 2 && piece >= 'a' && piece <= 'z') ||
                (joueur == 1 && piece >= 'A' && piece <= 'Z')) {
                char posAttaque[3] = {'a' + y, '0' + (8 - x), '\0'};
                char posRoi[3] = {'a' + roiY, '0' + (8 - roiX), '\0'};
                int peutAttaquer = 0;
                if (piece == 'p' || piece == 'P') peutAttaquer = pion(PLATEAU, posAttaque, posRoi, 1, state);
                else if (piece == 'f' || piece == 'F') peutAttaquer = fou(PLATEAU, posAttaque, posRoi, 1);
                else if (piece == 't' || piece == 'T') peutAttaquer = tour(PLATEAU, posAttaque, posRoi, 1);
                else if (piece == 'c' || piece == 'C') peutAttaquer = cavalier(PLATEAU, posAttaque, posRoi, 1);
                else if (piece == 'q' || piece == 'Q') peutAttaquer = Queen(PLATEAU, posAttaque, posRoi, 1);
                else if (piece == 'k' || piece == 'K') peutAttaquer = King(PLATEAU, posAttaque, posRoi, 1, state);
                if (peutAttaquer) {
                    return 1;
                }
            }
        }
    }

    return 0;
}

// Vérification d'échec et mat
int estEchecEtMat(char PLATEAU[8][8], int joueur, GameState *state) {
    for (int x1 = 0; x1 < 8; x1++) {
        for (int y1 = 0; y1 < 8; y1++) {
            char piece = PLATEAU[x1][y1];
            if ((joueur == 1 && piece >= 'a' && piece <= 'z') ||
                (joueur == 2 && piece >= 'A' && piece <= 'Z')) {
                for (int x2 = 0; x2 < 8; x2++) {
                    for (int y2 = 0; y2 < 8; y2++) {
                        char start[3] = {'a' + y1, '0' + (8 - x1), '\0'};
                        char end[3] = {'a' + y2, '0' + (8 - x2), '\0'};
                        char copie[8][8];
                        memcpy(copie, PLATEAU, sizeof(char) * 64);
                        GameState stateCopy = *state;
                        if (deplacerPiece(copie, start, end, 1, 1, &stateCopy)) {
                            if (!echec(copie, joueur, &stateCopy)) {
                                return 0;
                            }
                        }
                    }
                }
            }
        }
    }
    return 1;
}

// Vérification de pat
int estPat(char PLATEAU[8][8], int joueur, GameState *state) {
    if (echec(PLATEAU, joueur, state)) return 0;

    for (int x1 = 0; x1 < 8; x1++) {
        for (int y1 = 0; y1 < 8; y1++) {
            char piece = PLATEAU[x1][y1];
            if ((joueur == 1 && piece >= 'a' && piece <= 'z') ||
                (joueur == 2 && piece >= 'A' && piece <= 'Z')) {
                for (int x2 = 0; x2 < 8; x2++) {
                    for (int y2 = 0; y2 < 8; y2++) {
                        char start[3] = {'a' + y1, '0' + (8 - x1), '\0'};
                        char end[3] = {'a' + y2, '0' + (8 - x2), '\0'};
                        char copie[8][8];
                        memcpy(copie, PLATEAU, sizeof(char) * 64);
                        GameState stateCopy = *state;
                        if (deplacerPiece(copie, start, end, 1, 1, &stateCopy)) {
                            return 0;
                        }
                    }
                }
            }
        }
    }
    return 1;
}

// Fonction principale de la partie
void partie(char PLATEAU[8][8], int Lar, int Long, GameState *state, int tourBlancs, int scoreBlancs, int scoreNoirs, int tourActuel) {
    afficherTableau(PLATEAU, Lar, Long);

    while (1) {
        printf("\nTour des %s (Score: Blancs %d, Noirs %d)\n", tourBlancs ? "Blancs" : "Noirs", scoreBlancs, scoreNoirs);
        int joueur = tourBlancs ? 2 : 1;

        if (estMatchNulMateriel(PLATEAU)) {
            printf("Match nul par insuffisance de matériel !\n");
            return;
        }

        if (echec(PLATEAU, joueur, state)) {
            printf("Échec !\n");
            if (estEchecEtMat(PLATEAU, joueur, state)) {
                printf("Échec et mat ! Les %s gagnent !\n", tourBlancs ? "Noirs" : "Blancs");
                return;
            }
        } else if (estPat(PLATEAU, joueur, state)) {
            printf("Pat ! Match nul.\n");
            return;
        }

        do {
            printf("Déplacer de (ex: e2) ou 'q' pour quitter : ");
            char start[3];
            scanf("%s", start);
            if (start[0] == 'q' || start[0] == 'Q') {
                printf("Partie terminée. Score final: Blancs %d, Noirs %d\n", scoreBlancs, scoreNoirs);
                return;
            }
            if (!validerEntree(start)) {
                printf("Entrée invalide (ex: e2).\n");
                continue;
            }

            int x1, y1;
            convertirPosition(start, &x1, &y1);
            char piece = PLATEAU[x1][y1];
            if ((tourBlancs && (piece < 'A' || piece > 'Z')) ||
                (!tourBlancs && (piece < 'a' || piece > 'z'))) {
                printf("Ce n'est pas votre pièce.\n");
                continue;
            }

            printf("Déplacer vers (ex: e4) : ");
            char end[3];
            scanf("%s", end);
            if (!validerEntree(end)) {
                printf("Entrée invalide (ex: e4).\n");
                continue;
            }

            int x2, y2;
            convertirPosition(end, &x2, &y2);
            char pieceCapturee = PLATEAU[x2][y2];
            int points = 0;
            if (pieceCapturee != ' ' && !((piece == 'K' || piece == 'k') && (strcmp(end, "g1") == 0 || strcmp(end, "c1") == 0 || strcmp(end, "g8") == 0 || strcmp(end, "c8") == 0))) {
                switch (pieceCapturee) {
                    case 'p': case 'P': points = 1; break;
                    case 'c': case 'C': case 'f': case 'F': points = 3; break;
                    case 't': case 'T': points = 5; break;
                    case 'q': case 'Q': points = 9; break;
                    default: points = 0;
                }
            }

            if (deplacerPiece(PLATEAU, start, end, 1, 0, state)) {
                if (tourBlancs) scoreBlancs += points;
                else scoreNoirs += points;
                int adversaire = tourBlancs ? 1 : 2;
                if (echec(PLATEAU, adversaire, state)) {
                    printf("Échec !\n");
                    if (estEchecEtMat(PLATEAU, adversaire, state)) {
                        printf("Échec et mat ! Les %s gagnent !\n", tourBlancs ? "Blancs" : "Noirs");
                        return;
                    }
                } else if (estPat(PLATEAU, adversaire, state)) {
                    printf("Pat ! Match nul.\n");
                    return;
                }
                tourBlancs = !tourBlancs;
                tourActuel++;
                break;
            } else {
                printf("Mouvement invalide.\n");
            }
        } while (1);

        afficherTableau(PLATEAU, Lar, Long);
    }
}

// Menu principal
void afficherMenu() {
    int choix;
    do {
        printf("\n=== Jeu d'Échecs ===\n");
        printf("1. Jouer contre un joueur\n");
        printf("2. Jouer contre une IA\n");
        printf("3. Afficher les règles\n");
        printf("4. Charger une partie sauvegardée\n");
        printf("5. Quitter\n");
        printf("Votre choix : ");
        scanf("%d", &choix);

        switch (choix) {
            case 1:
                jouerContreJoueur();
                break;
            case 2:
                jouerContreIA();
                break;
            case 3:
                afficherRegles();
                break;
            case 4:
                chargerPartieSauvegardee();
                break;
            case 5:
                printf("Au revoir !\n");
                exit(0);
            default:
                printf("Choix invalide. Veuillez choisir une option entre 1 et 5.\n");
        }
    } while (1);
}

void jouerContreJoueur() {
    char plateau[8][8];
    int Lar = 8, Long = 8;
    GameState state;
    iniTableau(plateau, Lar, Long, &state);
    partie(plateau, Lar, Long, &state, 1, 0, 0, 0);
}

void jouerContreIA() {
    char plateau[8][8];
    int Lar = 8, Long = 8;
    GameState state;
    iniTableau(plateau, Lar, Long, &state);
    int tourBlancs = 1;
    int scoreBlancs = 0, scoreNoirs = 0;
    int tourActuel = 0;

    afficherTableau(plateau, Lar, Long);

    while (1) {
        printf("\nTour des %s (Score: Blancs %d, Noirs %d)\n", tourBlancs ? "Blancs" : "Noirs", scoreBlancs, scoreNoirs);
        int joueur = tourBlancs ? 2 : 1;

        if (estMatchNulMateriel(plateau)) {
            printf("Match nul par insuffisance de matériel !\n");
            return;
        }

        if (echec(plateau, joueur, &state)) {
            printf("Échec !\n");
            if (estEchecEtMat(plateau, joueur, &state)) {
                printf("Échec et mat ! Les %s gagnent !\n", tourBlancs ? "Noirs" : "Blancs");
                return;
            }
        } else if (estPat(plateau, joueur, &state)) {
            printf("Pat ! Match nul.\n");
            return;
        }

        if (tourBlancs) {
            // Tour du joueur (Blancs)
            do {
                printf("Déplacer de (ex: e2) ou 'q' pour quitter : ");
                char start[3];
                scanf("%s", start);
                if (start[0] == 'q' || start[0] == 'Q') {
                    printf("Partie terminée. Score final: Blancs %d, Noirs %d\n", scoreBlancs, scoreNoirs);
                    return;
                }
                if (!validerEntree(start)) {
                    printf("Entrée invalide (ex: e2).\n");
                    continue;
                }

                int x1, y1;
                convertirPosition(start, &x1, &y1);
                char piece = plateau[x1][y1];
                if (piece < 'A' || piece > 'Z') {
                    printf("Ce n'est pas votre pièce.\n");
                    continue;
                }

                printf("Déplacer vers (ex: e4) : ");
                char end[3];
                scanf("%s", end);
                if (!validerEntree(end)) {
                    printf("Entrée invalide (ex: e4).\n");
                    continue;
                }

                int x2, y2;
                convertirPosition(end, &x2, &y2);
                char pieceCapturee = plateau[x2][y2];
                int points = 0;
                if (pieceCapturee != ' ' && !((piece == 'K' || piece == 'k') && (strcmp(end, "g1") == 0 || strcmp(end, "c1") == 0 || strcmp(end, "g8") == 0 || strcmp(end, "c8") == 0))) {
                    switch (pieceCapturee) {
                        case 'p': case 'P': points = 1; break;
                        case 'c': case 'C': case 'f': case 'F': points = 3; break;
                        case 't': case 'T': points = 5; break;
                        case 'q': case 'Q': points = 9; break;
                        default: points = 0;
                    }
                }

                if (deplacerPiece(plateau, start, end, 1, 0, &state)) {
                    scoreBlancs += points;
                    break;
                } else {
                    printf("Mouvement invalide.\n");
                }
            } while (1);
        } else {
            // Tour de l'IA (Noirs)
            printf("L'IA réfléchit...\n");
            Mouvement mouvements[4096]; // Taille suffisante pour stocker tous les mouvements possibles
            int nombreMouvements = trouverMouvementsLegaux(plateau, 1, &state, mouvements, 4096);
            if (nombreMouvements == 0) {
                printf("L'IA n'a plus de mouvements légaux !\n");
                return;
            }

            Mouvement mouvementChoisi;
            if (choisirMouvementAleatoire(mouvements, nombreMouvements, &mouvementChoisi)) {
                printf("L'IA déplace %s à %s\n", mouvementChoisi.start, mouvementChoisi.end);
                int x2, y2;
                convertirPosition(mouvementChoisi.end, &x2, &y2);
                char pieceCapturee = plateau[x2][y2];
                int points = 0;
                if (pieceCapturee != ' ' && !((pieceCapturee == 'K' || pieceCapturee == 'k') && (strcmp(mouvementChoisi.end, "g1") == 0 || strcmp(mouvementChoisi.end, "c1") == 0 || strcmp(mouvementChoisi.end, "g8") == 0 || strcmp(mouvementChoisi.end, "c8") == 0))) {
                    switch (pieceCapturee) {
                        case 'p': case 'P': points = 1; break;
                        case 'c': case 'C': case 'f': case 'F': points = 3; break;
                        case 't': case 'T': points = 5; break;
                        case 'q': case 'Q': points = 9; break;
                        default: points = 0;
                    }
                }
                deplacerPiece(plateau, mouvementChoisi.start, mouvementChoisi.end, 1, 0, &state);
                scoreNoirs += points;
            }
        }

        tourBlancs = !tourBlancs;
        tourActuel++;
        afficherTableau(plateau, Lar, Long);

        // Vérifier les conditions de fin après le mouvement de l'IA
        int adversaire = tourBlancs ? 1 : 2;
        if (echec(plateau, adversaire, &state)) {
            printf("Échec !\n");
            if (estEchecEtMat(plateau, adversaire, &state)) {
                printf("Échec et mat ! Les %s gagnent !\n", tourBlancs ? "Blancs" : "Noirs");
                return;
            }
        } else if (estPat(plateau, adversaire, &state)) {
            printf("Pat ! Match nul.\n");
            return;
        }
    }
}

void afficherRegles() {
    printf("\n=== Règles des Échecs ===\n");
    printf("1 - Le jeu se joue sur un plateau 8x8 avec des pièces blanches et noires.\n");
    printf("2 - Les pièces incluent : Roi (K/k), Reine (Q/q), Tour (T/t), Fou (F/f), Cavalier (C/c), Pion (P/p).\n");
    printf("3 - Les Blancs jouent en premier, puis les joueurs alternent.\n");
    printf("4 - Prise en passant** : Si un pion avance de deux cases et passe à côté d'un pion adverse, ce dernier peut le capturer au tour suivant.\n");
    printf("5 - Roque : Le roi peut se déplacer de deux cases vers une tour si ni l'un ni l'autre n'a bougé et que le chemin est libre.\n");
    printf("6 - Promotion : Un pion atteignant la dernière rangée peut être promu en reine, tour, fou ou cavalier.\n");
    printf("7 - Échec et mat : Si un roi est en échec et ne peut pas échapper, la partie est terminée.\n");
    printf("8 - Pat : Si un joueur n'a aucun mouvement légal et que son roi n'est pas en échec, la partie est nulle.\n");
    printf("Entrez une touche pour revenir au menu.\n");
    getchar(); // Pour consommer le '\n' restant
    getchar();
}

// Fonctions pour initialiser les parties sauvegardées
void initialiserPartiePriseEnPassant(SavedGame *game) {
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            game->plateau[i][j] = ' ';
        }
    }
    game->plateau[7][4] = 'K'; // Roi blanc sur e1
    game->plateau[0][4] = 'k'; // Roi noir sur e8
    game->plateau[6][4] = 'P'; // Pion blanc sur e2
    game->plateau[4][3] = 'p'; // Pion noir sur d4
    game->state.roiBlancBouge = 0;
    game->state.tourBlancheA1Bouge = 0;
    game->state.tourBlancheH1Bouge = 0;
    game->state.roiNoirBouge = 0;
    game->state.tourNoireA8Bouge = 0;
    game->state.tourNoireH8Bouge = 0;
    game->state.enPassantX = -1;
    game->state.enPassantY = -1;
    game->state.dernierTourDouble = -1;
    game->tourBlancs = 1;
    game->scoreBlancs = 0;
    game->scoreNoirs = 0;
    game->tourActuel = 0;
}

void initialiserPartieRoque(SavedGame *game) {
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            game->plateau[i][j] = ' ';
        }
    }
    game->plateau[7][0] = 'T'; game->plateau[7][4] = 'K'; game->plateau[7][7] = 'T'; // Tours et roi blanc
    game->plateau[0][0] = 't'; game->plateau[0][4] = 'k'; game->plateau[0][7] = 't'; // Tours et roi noir
    game->plateau[4][2] = 'F';
    game->state.roiBlancBouge = 0;
    game->state.tourBlancheA1Bouge = 0;
    game->state.tourBlancheH1Bouge = 0;
    game->state.roiNoirBouge = 0;
    game->state.tourNoireA8Bouge = 0;
    game->state.tourNoireH8Bouge = 0;
    game->state.enPassantX = -1;
    game->state.enPassantY = -1;
    game->state.dernierTourDouble = -1;
    game->tourBlancs = 1;
    game->scoreBlancs = 0;
    game->scoreNoirs = 0;
    game->tourActuel = 0;
}

void initialiserPartiePromotion(SavedGame *game) {
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            game->plateau[i][j] = ' ';
        }
    }
    game->plateau[7][4] = 'K'; // Roi blanc sur e1
    game->plateau[0][7] = 'k'; // Roi noir sur e8
    game->plateau[1][4] = 'P'; // Pion blanc sur e7 (prêt à être promu)
    game->state.roiBlancBouge = 0;
    game->state.tourBlancheA1Bouge = 0;
    game->state.tourBlancheH1Bouge = 0;
    game->state.roiNoirBouge = 0;
    game->state.tourNoireA8Bouge = 0;
    game->state.tourNoireH8Bouge = 0;
    game->state.enPassantX = -1;
    game->state.enPassantY = -1;
    game->state.dernierTourDouble = -1;
    game->tourBlancs = 1;
    game->scoreBlancs = 0;
    game->scoreNoirs = 0;
    game->tourActuel = 0;
}

void initialiserPartieEchecEtMat(SavedGame *game) {
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            game->plateau[i][j] = ' ';
        }
    }
    game->plateau[7][4] = 'K'; // Roi blanc sur e1
    game->plateau[0][4] = 'k'; // Roi noir sur e8
    game->plateau[2][3] = 'Q'; // Reine blanche sur d6
    game->plateau[2][5] = 'F'; // Fou blanc sur f6
    game->state.roiBlancBouge = 0;
    game->state.tourBlancheA1Bouge = 0;
    game->state.tourBlancheH1Bouge = 0;
    game->state.roiNoirBouge = 0;
    game->state.tourNoireA8Bouge = 0;
    game->state.tourNoireH8Bouge = 0;
    game->state.enPassantX = -1;
    game->state.enPassantY = -1;
    game->state.dernierTourDouble = -1;
    game->tourBlancs = 1;
    game->scoreBlancs = 0;
    game->scoreNoirs = 0;
    game->tourActuel = 0;
}

void initialiserPartiePat(SavedGame *game) {
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            game->plateau[i][j] = ' ';
        }
    }
    game->plateau[0][0] = 'K'; // Roi blanc sur a1
    game->plateau[0][4] = 'k'; // Roi noir sur e8
    game->plateau[3][2] = 'q'; // Reine blanche sur b7
    game->state.roiBlancBouge = 0;
    game->state.tourBlancheA1Bouge = 0;
    game->state.tourBlancheH1Bouge = 0;
    game->state.roiNoirBouge = 0;
    game->state.tourNoireA8Bouge = 0;
    game->state.tourNoireH8Bouge = 0;
    game->state.enPassantX = -1;
    game->state.enPassantY = -1;
    game->state.dernierTourDouble = -1;
    game->tourBlancs = 0; // Tour des Noirs
    game->scoreBlancs = 0;
    game->scoreNoirs = 0;
    game->tourActuel = 0;
}

void chargerPartieSauvegardee() {
    int choix;
    SavedGame game;
    do {
        printf("\n=== Parties Sauvegardees ===\n");
        printf("1. Prise en passant\n");
        printf("2. Roque\n");
        printf("3. Promotion\n");
        printf("4. Échec et mat\n");
        printf("5. Pat\n");
        printf("6. Retour au menu principal\n");
        printf("Votre choix : ");
        scanf("%d", &choix);

        switch (choix) {
            case 1:
                initialiserPartiePriseEnPassant(&game);
                break;
            case 2:
                initialiserPartieRoque(&game);
                break;
            case 3:
                initialiserPartiePromotion(&game);
                break;
            case 4:
                initialiserPartieEchecEtMat(&game);
                break;
            case 5:
                initialiserPartiePat(&game);
                break;
            case 6:
                return;
            default:
                printf("Choix invalide. Veuillez choisir une option entre 1 et 6.\n");
                continue;
        }

        printf("\n%s\n", game.description);
        partie(game.plateau, 8, 8, &game.state, game.tourBlancs, game.scoreBlancs, game.scoreNoirs, game.tourActuel);
    } while (1);
}

// Trouver tous les mouvements légaux pour un joueur
int trouverMouvementsLegaux(char PLATEAU[8][8], int joueur, GameState *state, Mouvement *mouvements, int maxMouvements) {
    int nombreMouvements = 0;
    for (int x1 = 0; x1 < 8; x1++) {
        for (int y1 = 0; y1 < 8; y1++) {
            char piece = PLATEAU[x1][y1];
            if ((joueur == 1 && piece >= 'a' && piece <= 'z') ||
                (joueur == 2 && piece >= 'A' && piece <= 'Z')) {
                for (int x2 = 0; x2 < 8; x2++) {
                    for (int y2 = 0; y2 < 8; y2++) {
                        char start[3] = {'a' + y1, '0' + (8 - x1), '\0'};
                        char end[3] = {'a' + y2, '0' + (8 - x2), '\0'};
                        char copie[8][8];
                        memcpy(copie, PLATEAU, sizeof(char) * 64);
                        GameState stateCopy = *state;
                        if (deplacerPiece(copie, start, end, 1, 1, &stateCopy)) {
                            if (nombreMouvements < maxMouvements) {
                                strcpy(mouvements[nombreMouvements].start, start);
                                strcpy(mouvements[nombreMouvements].end, end);
                                nombreMouvements++;
                            }
                        }
                    }
                }
            }
        }
    }
    return nombreMouvements;
}

// Choisir un mouvement aléatoire parmi les mouvements légaux
int choisirMouvementAleatoire(Mouvement *mouvements, int nombreMouvements, Mouvement *mouvementChoisi) {
    if (nombreMouvements == 0) return 0;
    srand(time(NULL)); // Initialiser le générateur aléatoire
    int index = rand() % nombreMouvements;
    *mouvementChoisi = mouvements[index];
    return 1;
}

// Fonction principale
int main() {
    afficherMenu();
    return 0;
}