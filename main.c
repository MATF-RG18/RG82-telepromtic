#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

/* Error-checking function. Used for technical C details */
#define osAssert(condition, msg) osError(condition, msg)
void osError(bool condition, const char* msg) {
    if (!condition) {
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

/* Maximum input file names */
#define MAX_FILE_NAME 32

/* Structure that will keep data for every field cube. 
 * 1) type can be: 'w' - wall, 'l' - lava, 't' - teleport, 'd' - door, 'e' - elevator,
 *    'k' - key, 's' - switch, 'X' - goal, '@' - player starting position
 * 2) color can be: 'r' - red, 'g' - green, 'b' - blue, 'y' - yellow, 'o' - orange,
 *    'p' - purple, 'c' - cyan, 'm' - magenta, 'z' - brown, 'q' - grey (we want all
 *     keys/doors to be brown and switches/elevators to be grey, to avoid too much colors)
 * 3) to_row and to_col will store indexes in map matrix for teleport-teleport, 
 *    key-door and switch/elevator that are connected
 * switch-elevator pairs that are connected 
 * 4) height stores height of the cube */
typedef struct field {
    char type;
    char color;
    int to_row, to_col;
    int height;
}   FieldData;

/* Height and width of the map */
static int map_rows, map_cols;
/* Metadata input file (map info about every cube) */
const static char map_input_file[MAX_FILE_NAME] = "map.txt";
/* Map dimensions file */
const static char map_dimensions_file[MAX_FILE_NAME] = "map_dimensions.txt";
/* Map connections and teleport colors file */
const static char map_connections_file[MAX_FILE_NAME] = "map_connections.txt";
/* Every part of the field is made of cube of fixed size */
const static float CUBE_SIZE = 0.6;

/* Camera position and look direction*/
static float eye_x, eye_y, eye_z;
static float to_x, to_y, to_z;

/* Moving parameter */
static float move_param = 0.02;

/* Main game matrix that will store basic info about every game cube */
static FieldData** map = NULL;

/*Basic glut callback functions declarations*/
static void on_keyboard(unsigned char key, int x, int y);
static void on_reshape(int width, int height);
static void on_display(void);

/* Function that alocates space for map matrix */
static FieldData** allocate_map();
/* Function that frees dymanicly allocated space for map matrix */
static FieldData** free_map();
/* Function that stores cube types and their heights, pulled from a .txt file. */
static void store_map_data();
/* Function that stores map field connections and teleport colors */
static void store_map_connections();
/* Function that physically creates map in the game */
static void create_map();

/* Basic GL/glut initialization */
static void glut_initialize();

/* Other initialization */
static void initialize();

int main(int argc, char** argv)
{
    /* Light parameters */
    GLfloat light_position[] = {1, 1, 1, 0};
    GLfloat light_ambient[] = {0.2, 0.2, 0.2, 1};
    GLfloat light_diffuse[] = {0.7, 0.7, 0.7, 1};
    GLfloat light_specular[] = {0.9, 0.9, 0.9, 1};

    /* Material light parameters */
    GLfloat ambient_coeffs[] = {0.2, 0.2, 0.2, 1};
    GLfloat diffuse_coeffs[] = {0.2, 0.8, 0, 1};
    GLfloat specular_coeffs[] = {0.3, 0.3, 0.3, 1};
    GLfloat shininess = 30;

    /* Basic GLUT initialization */
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);

    /* Window settings */
    glutInitWindowSize(600, 600);
    glutInitWindowPosition(100, 100);
    glutCreateWindow(argv[0]);

    /* Registrating glut callback functions */
    glutKeyboardFunc(on_keyboard);
    glutReshapeFunc(on_reshape);
    glutDisplayFunc(on_display);

    /* Initializing basic clear color, depth test and initial var values */
    glut_initialize();

    /* Initializing map dimensions, camera parameters ... */
    initialize();

    /* Putting (directional) light on the scene */
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    /* Setting up light parameters */
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);

    /* Setting up material parameters */
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient_coeffs);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse_coeffs);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular_coeffs);
    glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, shininess);

    map = allocate_map();
    store_map_data();
    store_map_connections();

    /*
    int i,j;
    for (i = 0; i < map_rows; i++) {
        for (j = 0; j < map_cols; j++)
            printf("%c%d ", map[i][j].type, map[i][j].height);
        putchar('\n');
    }
    
    printf("CUBE_SIZE: %f\n", CUBE_SIZE);


    printf("\n------------------- Connections ---------------------\n");
    for (i = 0; i < map_rows; i++) {
        for (j = 0; j < map_cols; j++)
            printf("[%d][%d]: (%c, %d, %d)\n", 
                i, j, map[i][j].color, map[i][j].to_row, map[i][j].to_col);
    }
    */

    /* Entering OpenGL main loop */
    glutMainLoop();

    map = free_map(map);

    return 0;
}

static void on_keyboard(unsigned char key, int x, int y)
{
    switch (key) {
        /* 'esc' exits the program */
        case 27:
            exit(0);
            break;
        case 'w':
        case 'W':
            eye_z += move_param;
            glutPostRedisplay();
            break;
        case 's':
        case 'S':
            eye_z -= move_param;
            glutPostRedisplay();
            break;
        case 'a':
        case 'A':
            eye_x -= move_param;
            glutPostRedisplay();
            break;
        case 'd':
        case 'D':
            eye_x += move_param;
            glutPostRedisplay();
            break;
    }
}

static void on_reshape(int width, int height)
{
    /* Setting view port and view perspective */
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, (float)width / height, 1, 10);
}

static void on_display(void)
{
    /* GLfloat light_position[] = {eye_x, eye_y, eye_z, 1}; */

    /* Clearing the previous window appearance */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* Cammera settings */
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    gluLookAt(eye_x, eye_y, eye_z,
              to_x, to_y, to_z,
              0, 1, 0);

    create_map();

    glutSwapBuffers();
}

static void glut_initialize()
{   
    glClearColor(0.7, 0.7, 0.7, 0);
    glEnable(GL_DEPTH_TEST);
}

static void initialize()
{
    eye_x = 0;
    eye_y = 5;
    eye_z = 0;
    to_x = 2;
    to_y = 2;
    to_z = 2;

    FILE* f = fopen(map_dimensions_file, "r");
    osAssert(f != NULL, "Error opening file \"map_dimensions.txt\"\n");

    fscanf(f, "%d %d", &map_rows, &map_cols);
    fclose(f);
}

static FieldData** allocate_map()
{
    FieldData** m = NULL;
    int i;

    m = (FieldData**)malloc(map_rows * sizeof(FieldData*));
    osAssert(m != NULL, "Allocating memory for map matrix rows failed\n");

    for(i = 0; i < map_rows; i++) {
        m[i] = (FieldData*)malloc(map_cols * sizeof(FieldData));
        if (m[i] == NULL) {
            int j;
            for (j = 0; j < i; j++)
                free(m[j]);

            free(m);

            fprintf(stderr, "Allocating memory for one of map matrix columns failed.");
            exit(EXIT_FAILURE);
        }
    }

    return m;
}

static FieldData** free_map()
{
    int i;
    for(i = 0; i < map_rows; i++)
        free(map[i]);

    free(map);
    return NULL;
}

static void store_map_data()
{
    FILE* f = NULL;
    int i, j;

    f = fopen(map_input_file, "r");
    osAssert(f != NULL, "Error opening file \"map.txt\"\n");

    for (i = 0; i < map_rows; i++) {
        for (j = 0; j < map_cols; j++) {
            fscanf(f, "%c%d ", &(*(map + i) + j)->type, &(*(map + i) + j)->height);
            /* Connection coords are initially -1: they will be updated later */
            map[i][j].to_row = map[i][j].to_col = -1;
            /* Color is initially 'u' - unsigned */
            map[i][j].color = 'u';
        }
    }

    fclose(f);
}

static void store_map_connections()
{
    FILE* f = NULL;
    int n, row1, row2, col1, col2, i;
    char c;

    f = fopen(map_connections_file, "r");
    osAssert(f != NULL, "Error opening file \"map_connections.txt\"\n");

    fscanf(f, "%d", &n);
    fgetc(f);
    for (i = 0; i < n; i++) {
        fscanf(f, "%c %d %d %d %d ", &c, &row1, &col1, &row2, &col2);
        map[row1][col1].color = c;
        map[row1][col1].to_row = row2;
        map[row1][col1].to_col = col2;
        /* And also backwards! */
        map[row2][col2].color = c;
        map[row2][col2].to_row = row1;
        map[row2][col2].to_col = col1;
    }

    fclose(f);
}

static void create_map()
{
    int i, j;

    glPushMatrix();
        glTranslatef(CUBE_SIZE / 2, - CUBE_SIZE / 2, CUBE_SIZE / 2);
        for (i = 0; i < map_rows; i++) {
            for (j = 0; j < map_cols; j++) {
                switch (map[i][j].type) {
                    case 'w':
                        if (map[i][j].height == 0) {
                            glPushMatrix();
                                glTranslatef(j*CUBE_SIZE, 0, i*CUBE_SIZE);
                                glColor3f(0.3, 0.8, 0.1);
                                glutSolidCube(CUBE_SIZE);
                            glPopMatrix();
                        } else {
                            glPushMatrix();
                                glScalef(1, (map[i][j].height + 1) * CUBE_SIZE, 1);
                                glTranslatef(j*CUBE_SIZE, 0, i*CUBE_SIZE);
                                glColor3f(0.6, 0.6, 0.1);
                                glutSolidCube(CUBE_SIZE);
                            glPopMatrix();
                        }
                        break;
                    case 'l':
                        glPushMatrix();
                            glTranslatef(j*CUBE_SIZE, 0, i*CUBE_SIZE);
                            glColor3f(0.8, 0.2, 0.1);
                            glutSolidCube(CUBE_SIZE);
                        glPopMatrix();
                        break;
                    default:
                        glPushMatrix();
                            glTranslatef(j*CUBE_SIZE, 0, i*CUBE_SIZE);
                            glColor3f(0.3, 0.8, 0.1);
                            glutSolidCube(CUBE_SIZE);
                        glPopMatrix();
                        break;
                }
            }
        }
    glPopMatrix();
}

