#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define MAX_WORDS 10
#define MAX_GRID_SIZE 15
#define MAX_WORD_LENGTH 20

#define COLOR_RED    "\x1b[31m"
#define COLOR_GREEN  "\x1b[32m"
#define COLOR_BLUE   "\x1b[34m"
#define COLOR_RESET  "\x1b[0m"

typedef struct {
    int row, col, dir, overlaps;
} Position;

char grid[MAX_GRID_SIZE][MAX_GRID_SIZE];
char displayGrid[MAX_GRID_SIZE][MAX_GRID_SIZE];
char words[MAX_WORDS][MAX_WORD_LENGTH];
int placedStartRow[MAX_WORDS], placedStartCol[MAX_WORDS], placedDir[MAX_WORDS];
int wordCount = 0;
int level;
int correctGuesses = 0;

typedef struct { int r, c; } Cell;
Cell allWordCells[MAX_WORDS * MAX_WORD_LENGTH];
int allWordCellCount = 0;
Cell unrevealedCells[MAX_WORDS * MAX_WORD_LENGTH];
int unrevealedCount = 0;
int lastHintWord = -1;

void playCorrectSound() { printf("\a"); fflush(stdout); }
void playIncorrectSound() { printf("\a\a"); fflush(stdout); }

void initializeGrid() {
    for (int i = 0; i < MAX_GRID_SIZE; i++)
        for (int j = 0; j < MAX_GRID_SIZE; j++)
            grid[i][j] = displayGrid[i][j] = '.';
}

void printGrid(char grid[MAX_GRID_SIZE][MAX_GRID_SIZE]) {
    printf("\n    ");
    for (int j = 0; j < MAX_GRID_SIZE; j++) printf("%2d ", j);
    printf("\n    ");
    for (int j = 0; j < MAX_GRID_SIZE; j++) printf("---");
    for (int i = 0; i < MAX_GRID_SIZE; i++) {
        printf("\n%2d |", i);
        for (int j = 0; j < MAX_GRID_SIZE; j++) {
            printf("%2c ", grid[i][j]);
        }
    }
    printf("\n");
}

void printRevealedLetters() {
    printf("Revealed letters: ");
    int found = 0;
    for (int i = 0; i < MAX_GRID_SIZE; i++) {
        for (int j = 0; j < MAX_GRID_SIZE; j++) {
            char ch = displayGrid[i][j];
            if (ch != '.' && ch != ' ') {
                printf("%c ", ch);
                found = 1;
            }
        }
    }
    if (!found) printf("None");
    printf("\n");
}

int canPlaceWord(int row, int col, int dir, const char *word) {
    int len = strlen(word);
    if (dir == 0 && col + len > MAX_GRID_SIZE) return 0;
    if (dir == 1 && row + len > MAX_GRID_SIZE) return 0;
    for (int i = 0; i < len; i++) {
        int r = row + (dir ? i : 0);
        int c = col + (dir ? 0 : i);
        if (grid[r][c] != '.' && grid[r][c] != word[i]) return 0;
    }
    return 1;
}

int countOverlaps(int row, int col, int dir, const char *word) {
    int len = strlen(word), overlaps = 0;
    for (int i = 0; i < len; i++) {
        int r = row + (dir ? i : 0);
        int c = col + (dir ? 0 : i);
        if (grid[r][c] == word[i]) overlaps++;
    }
    return overlaps;
}

void doPlaceWord(int row, int col, int dir, const char *word, char placed[]) {
    int len = strlen(word);
    for (int i = 0; i < len; i++) {
        int r = row + (dir ? i : 0);
        int c = col + (dir ? 0 : i);
        if (grid[r][c] == '.') {
            grid[r][c] = word[i];
            placed[i] = 1;
        } else {
            placed[i] = 0;
        }
    }
}

void undoPlaceWord(int row, int col, int dir, const char *word, char placed[]) {
    int len = strlen(word);
    for (int i = 0; i < len; i++) {
        int r = row + (dir ? i : 0);
        int c = col + (dir ? 0 : i);
        if (placed[i]) grid[r][c] = '.';
    }
}

int placeWordsRec(int w) {
    if (w == wordCount) return 1;
    int len = strlen(words[w]);
    Position positions[MAX_GRID_SIZE * MAX_GRID_SIZE * 2];
    int posCount = 0;

    for (int dir = 0; dir < 2; dir++) {
        int maxRow = dir ? MAX_GRID_SIZE - len + 1 : MAX_GRID_SIZE;
        int maxCol = dir ? MAX_GRID_SIZE : MAX_GRID_SIZE - len + 1;
        for (int row = 0; row < maxRow; row++) {
            for (int col = 0; col < maxCol; col++) {
                if (canPlaceWord(row, col, dir, words[w])) {
                    int ov = countOverlaps(row, col, dir, words[w]);
                    positions[posCount].row = row;
                    positions[posCount].col = col;
                    positions[posCount].dir = dir;
                    positions[posCount].overlaps = ov;
                    posCount++;
                }
            }
        }
    }

    // Sort positions by overlaps descending
    for (int i = 0; i < posCount-1; i++) {
        for (int j = i+1; j < posCount; j++) {
            if (positions[j].overlaps > positions[i].overlaps) {
                Position tmp = positions[i];
                positions[i] = positions[j];
                positions[j] = tmp;
            }
        }
    }

    // Shuffle among equal-overlap positions for randomness
    int i = 0;
    while (i < posCount) {
        int j = i+1;
        while (j < posCount && positions[j].overlaps == positions[i].overlaps) j++;
        for (int k = j-1; k > i; k--) {
            int m = i + rand() % (k-i+1);
            Position tmp = positions[k];
            positions[k] = positions[m];
            positions[m] = tmp;
        }
        i = j;
    }

    for (int i = 0; i < posCount; i++) {
        int row = positions[i].row, col = positions[i].col, dir = positions[i].dir;
        char placed[MAX_WORD_LENGTH] = {0};
        doPlaceWord(row, col, dir, words[w], placed);
        placedStartRow[w] = row;
        placedStartCol[w] = col;
        placedDir[w] = dir;
        if (placeWordsRec(w+1)) return 1;
        undoPlaceWord(row, col, dir, words[w], placed);
    }
    return 0;
}

void placeWords() {
    if (!placeWordsRec(0)) {
        printf("Could not fit all words in the grid. Try fewer/shorter words or a bigger grid.\n");
        exit(1);
    }
}

// Loads words from file according to difficulty level
int selectWordsFromFile(char words[MAX_WORDS][MAX_WORD_LENGTH], int level) {
    FILE *file = NULL;
    char *filenames[] = {"easy_words.txt", "medium_words.txt", "hard_words.txt"};
    char tempWords[100][MAX_WORD_LENGTH];
    int totalWords = 0;

    if (level < 1 || level > 3) {
        printf("Invalid difficulty level.\n");
        return -1;
    }
    file = fopen(filenames[level-1], "r");
    if (!file) {
        printf(COLOR_RED "Error: Could not open %s\n" COLOR_RESET, filenames[level-1]);
        exit(1);
    }
    while (fscanf(file, "%19s", tempWords[totalWords]) == 1 && totalWords < 100) {
        totalWords++;
    }
    fclose(file);

    if (totalWords < MAX_WORDS) {
        printf(COLOR_RED "Not enough words in %s (need at least %d)\n" COLOR_RESET, filenames[level-1], MAX_WORDS);
        exit(1);
    }

    // Shuffle and select MAX_WORDS
    srand((unsigned)time(NULL));
    for (int i = totalWords-1; i > 0; i--) {
        int j = rand() % (i+1);
        char tmp[MAX_WORD_LENGTH];
        strcpy(tmp, tempWords[i]);
        strcpy(tempWords[i], tempWords[j]);
        strcpy(tempWords[j], tmp);
    }
    for (int i = 0; i < MAX_WORDS; i++) {
        strcpy(words[i], tempWords[i]);
    }
    wordCount = MAX_WORDS;
    return wordCount;
}

void gatherAllWordCells() {
    allWordCellCount = 0;
    for (int w = 0; w < wordCount; w++) {
        int len = strlen(words[w]);
        int r0 = placedStartRow[w];
        int c0 = placedStartCol[w];
        int dir = placedDir[w];
        for (int i = 0; i < len; i++) {
            int r = r0 + (dir ? i : 0);
            int c = c0 + (dir ? 0 : i);
            int unique = 1;
            for (int k = 0; k < allWordCellCount; k++)
                if (allWordCells[k].r == r && allWordCells[k].c == c) unique = 0;
            if (unique) {
                allWordCells[allWordCellCount].r = r;
                allWordCells[allWordCellCount].c = c;
                allWordCellCount++;
            }
        }
    }
}

void generateHints() {
    gatherAllWordCells();
    for (int i = 0; i < allWordCellCount; i++)
        unrevealedCells[i] = allWordCells[i];
    unrevealedCount = allWordCellCount;

    srand((unsigned)time(NULL));
    for (int i = allWordCellCount-1; i > 0; i--) {
        int j = rand() % (i+1);
        Cell temp = allWordCells[i];
        allWordCells[i] = allWordCells[j];
        allWordCells[j] = temp;
    }

    int hintsToShow = (int)ceil(allWordCellCount * 0.3); // 30% hints
    for (int i = 0; i < allWordCellCount; i++)
        displayGrid[allWordCells[i].r][allWordCells[i].c] = (i < hintsToShow) ? grid[allWordCells[i].r][allWordCells[i].c] : '.';

    int newUnrevealed = 0;
    for (int i = hintsToShow; i < allWordCellCount; i++)
        unrevealedCells[newUnrevealed++] = allWordCells[i];
    unrevealedCount = newUnrevealed;
}

void revealOneMoreHintFromDifferentWord() {
    if (unrevealedCount == 0) return;

    int wordHasHint[MAX_WORDS] = {0};
    for (int i = 0; i < unrevealedCount; i++) {
        for (int w = 0; w < wordCount; w++) {
            int len = strlen(words[w]);
            int r0 = placedStartRow[w], c0 = placedStartCol[w], dir = placedDir[w];
            for (int k = 0; k < len; k++) {
                int r = r0 + (dir ? k : 0);
                int c = c0 + (dir ? 0 : k);
                if (unrevealedCells[i].r == r && unrevealedCells[i].c == c) {
                    wordHasHint[w] = 1;
                }
            }
        }
    }

    int nextWord = (lastHintWord + 1) % wordCount;
    int tried = 0;
    while (!wordHasHint[nextWord] && tried < wordCount) {
        nextWord = (nextWord + 1) % wordCount;
        tried++;
    }
    if (!wordHasHint[nextWord]) {
        int idx = rand() % unrevealedCount;
        int r = unrevealedCells[idx].r, c = unrevealedCells[idx].c;
        displayGrid[r][c] = grid[r][c];
        unrevealedCells[idx] = unrevealedCells[unrevealedCount-1];
        unrevealedCount--;
        return;
    }

    int candidateIdx[MAX_WORDS * MAX_WORD_LENGTH], candCount = 0;
    for (int i = 0; i < unrevealedCount; i++) {
        int len = strlen(words[nextWord]);
        int r0 = placedStartRow[nextWord], c0 = placedStartCol[nextWord], dir = placedDir[nextWord];
        for (int k = 0; k < len; k++) {
            int r = r0 + (dir ? k : 0);
            int c = c0 + (dir ? 0 : k);
            if (unrevealedCells[i].r == r && unrevealedCells[i].c == c) {
                candidateIdx[candCount++] = i;
                break;
            }
        }
    }
    if (candCount > 0) {
        int idx = candidateIdx[rand() % candCount];
        int r = unrevealedCells[idx].r, c = unrevealedCells[idx].c;
        displayGrid[r][c] = grid[r][c];
        unrevealedCells[idx] = unrevealedCells[unrevealedCount-1];
        unrevealedCount--;
        lastHintWord = nextWord;
    }
}

void revealWholeWord(int w) {
    int len = strlen(words[w]);
    int r0 = placedStartRow[w];
    int c0 = placedStartCol[w];
    int dir = placedDir[w];
    for (int i = 0; i < len; i++) {
        int r = r0 + (dir ? i : 0);
        int c = c0 + (dir ? 0 : i);
        displayGrid[r][c] = grid[r][c];
        for (int k = 0; k < unrevealedCount; k++) {
            if (unrevealedCells[k].r == r && unrevealedCells[k].c == c) {
                unrevealedCells[k] = unrevealedCells[unrevealedCount - 1];
                unrevealedCount--;
                break;
            }
        }
    }
}

void printTimer(int secondsLeft, int totalTime) {
    int min = secondsLeft / 60, sec = secondsLeft % 60;
    printf("Time left: %02d:%02d ", min, sec);
    int barWidth = 30;
    int filled = (int)((barWidth * secondsLeft) / (float)totalTime);
    printf("[");
    for (int i = 0; i < barWidth; i++) {
        if (i < filled) printf("#");
        else printf("-");
    }
    printf("]\n");
}

void printRemainingWords(int solved[], int wordCount, char words[MAX_WORDS][MAX_WORD_LENGTH]) {
    printf("Words remaining: ");
    int any = 0;
    for (int i = 0; i < wordCount; i++) {
        if (!solved[i]) {
            printf("%s ", words[i]);
            any = 1;
        }
    }
    if (!any) printf("None!");
    printf("\n");
}

// Returns 1 if all letters of word w are revealed in displayGrid
int isWordFullyRevealed(int w) {
    int len = strlen(words[w]);
    int r0 = placedStartRow[w];
    int c0 = placedStartCol[w];
    int dir = placedDir[w];
    for (int i = 0; i < len; i++) {
        int r = r0 + (dir ? i : 0);
        int c = c0 + (dir ? 0 : i);
        if (displayGrid[r][c] != grid[r][c])
            return 0;
    }
    return 1;
}

int main() {
    srand((unsigned)time(NULL));
    printf("Difficulty Level (1=Easy, 2=Medium, 3=Hard): ");
    scanf("%d", &level);

    selectWordsFromFile(words, level);

    printf("\nSelected Words (in random order):\n");
    for (int i = 0; i < wordCount; i++) {
        printf("%s ", words[i]);
    }
    printf("\n");

    initializeGrid();
    placeWords();
    generateHints();

    printf("\n=== Crossword Puzzle ===\n");
    printGrid(displayGrid);

    printf("\nYou have 5 minutes to solve the crossword!\n");

    time_t start = time(NULL);
    time_t lastHint = start;
    int totalTime = 300; // 5 minutes
    int solved[MAX_WORDS] = {0};

    while (1) {
        int allSolved = 1;
        for (int w = 0; w < wordCount; w++) {
            if (!solved[w]) allSolved = 0;
        }
        if (allSolved) break;

        time_t now = time(NULL);
        int elapsed = (int)difftime(now, start);
        int secondsLeft = totalTime - elapsed;

        if (secondsLeft <= 0) {
            printf("\nTime's up!\n");
            break;
        }

        if (difftime(now, lastHint) >= 15 && unrevealedCount > 0) {
            revealOneMoreHintFromDifferentWord();
            lastHint = now;
            printf("\n[Hint revealed!]\n");
        }

        printTimer(secondsLeft, totalTime);
        printf(COLOR_BLUE "Score: %d/%d\n" COLOR_RESET, correctGuesses, wordCount);
        printGrid(displayGrid);
        printRevealedLetters();
        printRemainingWords(solved, wordCount, words);

        for (int w = 0; w < wordCount; w++) {
            if (!solved[w] && isWordFullyRevealed(w)) {
                solved[w] = 1;
                correctGuesses++;
                printf(COLOR_GREEN "Word \"%s\" was fully revealed by hints! Marked as solved.\n" COLOR_RESET, words[w]);
            }
        }

        int nextWord = -1;
        for (int w = 0; w < wordCount; w++) {
            if (!solved[w] && !isWordFullyRevealed(w)) {
                nextWord = w;
                break;
            }
        }
        if (nextWord == -1) continue;

        printf("\nWord to place: %s\n", words[nextWord]);
        int r, c, dir;
        printf("Row (0-%d): ", MAX_GRID_SIZE-1);
        scanf("%d", &r);
        printf("Column (0-%d): ", MAX_GRID_SIZE-1);
        scanf("%d", &c);
        printf("Direction (0-Horizontal, 1-Vertical): ");
        scanf("%d", &dir);

        if (placedStartRow[nextWord] == r && placedStartCol[nextWord] == c && placedDir[nextWord] == dir) {
            printf(COLOR_GREEN "Correct!\n" COLOR_RESET);
            playCorrectSound();
            revealWholeWord(nextWord);
            correctGuesses++;
            solved[nextWord] = 1;
        } else {
            printf(COLOR_RED "Incorrect. Try again.\n" COLOR_RESET);
            playIncorrectSound();
        }
    }

    printf("\n=== Final Solution ===\n");
    printGrid(grid);
    printf("\nYou solved %d out of %d words.\n", correctGuesses, wordCount);

    return 0;
}