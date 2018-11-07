#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_FILE_NAME 32

/* Structure that will keep data for every field cube. 
 * 1) type can be: 'w' - wall, 'l' - lava, 't' - teleport, 'd' - door, 'e' - elevator,
 * 'k' - key, 's' - switch, 'X' - goal, '@' - player starting position
 * 2) x, y, z will store coordinates for teleport-teleport, key-door and
 * switch-elevator pairs that are connected */
typedef struct field {
    char type;
    int x, y, z;
    int height;
}   FieldData;

/* Height and width of the map */
static int map_rows, map_cols;
/* Metadata input file (map info about every cube) */
const static char map_input_file[MAX_FILE_NAME] = "map.txt";
/* Map dimensions file */
const static char map_dimensions_file[MAX_FILE_NAME] = "map_dimensions.txt";
/* Every part of the field is made of cube of fixed size */
const static float CUBE_SIZE = 0.6;

/* Camera position and look direction*/
static float eye_x, eye_y, eye_z;
static float to_x, to_y, to_z;

/* Main game matrix that will store basic info about every game cube */
static FieldData** map = NULL;
/* Function that alocates space for map matrix */
static FieldData** allocate_map();
/* Function that frees dymanicly allocated space for map matrix */
static FieldData** free_map();
/* Function that stores cube types and their heights, pulled from a .txt file.
 * However, connections between cubes such as teleports still remain unchanged */
static void store_map_data();
/* Function that physically creates map in the game */
static void create_map();

/* Basic initializations */
static void initialize();

/*Basic glut callback functions declarations*/
static void on_keyboard(unsigned char key, int x, int y);
static void on_reshape(int width, int height);
static void on_display(void);

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
    GLfloat specular_coeffs[] = {0.1, 0.1, 0.1, 1};
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
    initialize();

    /* Putting (directional) light on the scene */
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient_coeffs);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse_coeffs);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular_coeffs);
    glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, shininess);

    map = allocate_map();
    store_map_data();

    int i,j;
    for (i = 0; i < map_rows; i++) {
        for (j = 0; j < map_cols; j++)
            printf("%c%d ", map[i][j].type, map[i][j].height);
        putchar('\n');
    }
    
    printf("CUBE_SIZE: %f\n", CUBE_SIZE);

    /* Entering OpenGL main loop */
    glutMainLoop();

    map = free_map(map);

    return 0;
}

static FieldData** allocate_map()
{
    FieldData** m = NULL;
    int i;

    m = (FieldData**)malloc(map_rows * sizeof(FieldData*));
    if (m == NULL) {
        fprintf(stderr, "Allocating memory for map matrix failed.");
        exit(EXIT_FAILURE);
    }

    for(i = 0; i < map_rows; i++) {
        m[i] = (FieldData*)malloc(map_cols * sizeof(FieldData));
        if (m[i] == NULL) {
            int j;
            for (j = 0; j < i; j++)
                free(m[j]);

            free(m);

            fprintf(stderr, "Allocating memory for map matrix columns failed.");
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
    for (i = 0; i < map_rows; i++) {
        for (j = 0; j < map_cols; j++) {
            fscanf(f, "%c%d ", &(*(map + i) + j)->type, &(*(map + i) + j)->height);
        }
    }

    fclose(f);
}

static void create_map()
{
    int i, j;

    glPushMatrix();
        glTranslatef(CUBE_SIZE / 2, CUBE_SIZE / 2, CUBE_SIZE / 2);
        for (i = 0; i < map_rows; i++) {
            for (j = 0; j < map_cols; j++) {
                glPushMatrix();
                    glTranslatef(j*CUBE_SIZE, 0, (map_rows - 1 - i)*CUBE_SIZE);
                    glutSolidCube(CUBE_SIZE);
                glPopMatrix();
            }
        }
    glPopMatrix();
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
    fscanf(f, "%d %d", &map_rows, &map_cols);
    fclose(f);

    glClearColor(0.7, 0.7, 0.7, 0);
    glEnable(GL_DEPTH_TEST);
}

static void on_keyboard(unsigned char key, int x, int y)
{
    switch (key) {
        /* 'esc' exits the program */
        case 27:
            exit(0);
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