#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>

#define MAX_FILE_NAME 32
#define PI 3.14159265359
#define EPS 0.01
#define RAD_TO_DEG 180/PI
#define DEG_TO_RAD PI/180

/* Error-checking function. Used for technical C details */
#define osAssert(condition, msg) osError(condition, msg)
void osError(bool condition, const char* msg) {
    if (!condition) {
        perror(msg);
        exit(EXIT_FAILURE);
    }
}


/* #define TELEPORT_TIMER_ID 1 */

/* Structure that will keep data for every field cube. 
 * 1) type can be: 'w' - wall, 'l' - lava, 't' - teleport, 'd' - door, 'e' - elevator,
 *    'k' - key, 's' - switch, 'X' - goal, '@' - player starting position
 * 2) color can be: 'r' - red, 'g' - green, 'b' - blue, 'y' - yellow, 'o' - orange,
 *    'p' - purple, 'c' - cyan, 'm' - magenta, 'z' - brown, 'q' - grey (we want all
 *     keys/doors to be brown and switches/elevators to be grey, to avoid too much colors)
 * 3) to_row and to_col will store indexes in map matrix for teleport-teleport, 
 *    key-door and switch/elevator that are connected
 * 4) height stores height of the cube: 0 height means floor */
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

/* Camera position, look direction and normal vector*/
static float eye_x, eye_y, eye_z;
static float to_x, to_y, to_z;
static float n_x, n_y, n_z;

/* Moving parameter */
static float move_param = 0.02;

/* Material component coeffs that will be updated by support function */
GLfloat coeffs[] = {0, 0, 0, 1};

/* Main game matrix that will store basic info about every game cube */
static FieldData** map = NULL;

/* Teleport time parameter and active indicator*/
/* static float teleport_time = 0; */
/* static bool teleport_timer_active = false; */

/*Basic glut callback functions declarations*/
static void on_keyboard(unsigned char key, int x, int y);
/* static void on_timer(int value); */
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
static void other_initialize();

/* Support function that sets coeffs in a global vector */
static void set_coeffs(float r, float g, float b, float a);

/* Support function that resets default material lighting settings */
/* static void reset_material(); */

/* Support function that draws coordinate system */
static void draw_axis();

/* Creates wall with h height */
static void creat_wall(float h);

/* Support functions that draw cylinder */
static void set_norm_vert_cylinder(float r, float phi, float h);
static void draw_cylinder(float r, float h);

/* Creates keys on the map */
static void create_key();

/* Creates teleport with the given color */
static void create_teleport(float x, float y, float z, char color);

int main(int argc, char** argv)
{
    /* Light parameters */
    GLfloat light_position[] = {1, 1, 1, 0};
    GLfloat light_ambient[] = {0.2, 0.2, 0.2, 1};
    GLfloat light_diffuse[] = {0.7, 0.7, 0.7, 1};
    GLfloat light_specular[] = {0.9, 0.9, 0.9, 1};

    /* Material lighting parameters */
    GLfloat ambient_coeffs[] = {0.2, 0.2, 0.2, 1};
    GLfloat diffuse_coeffs[] = {0.5, 0.5, 0.5, 1};
    GLfloat specular_coeffs[] = {0.3, 0.3, 0.3, 1};
    GLfloat shininess = 20;

    /* Basic GLUT initialization */
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);

    /* Window settings */
    glutInitWindowSize(800, 600);
    glutInitWindowPosition(100, 100);
    glutCreateWindow(argv[0]);

    /* Registrating glut callback functions */
    glutKeyboardFunc(on_keyboard);
    glutReshapeFunc(on_reshape);
    glutDisplayFunc(on_display);

    /* Initializing basic clear color, depth test, lighting, materials ... */
    glut_initialize();

    /* Initializing map dimensions, camera parameters ... */
    other_initialize();

    /* Putting (directional) light on the scene */
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    /* Setting up light parameters */
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);

    /* Setting up default material parameters */
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient_coeffs);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse_coeffs);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular_coeffs);
    glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, shininess);

    /* Storing map data */
    map = allocate_map();
    store_map_data();
    store_map_connections();


    /* Entering OpenGL main loop */
    glutMainLoop();

    /* Freeing dynamicly allocated space for map */
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
            eye_z -= move_param;
            glutPostRedisplay();
            break;

        case 's':
        case 'S':
            eye_z += move_param;
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

        case 'r':
        case 'R':
            eye_x = 0;
            eye_y = 8*CUBE_SIZE;
            eye_z = 0;
            glutPostRedisplay();
            break;
    }
}

/*
static void on_timer(int value)
{
    if (value != TELEPORT_TIMER_ID) {
        return;
    }

    teleport_time += 0.01;

    glutPostRedisplay();

    glutTimerFunc(10, on_timer, TELEPORT_TIMER_ID);
}
*/

static void on_reshape(int width, int height)
{
    /* Setting view port and view perspective */
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, (float)width / height, 1, 15);
}

static void glut_initialize()
{   
    glClearColor(0.7, 0.7, 0.7, 0);
    glEnable(GL_DEPTH_TEST);

    /* Normal vector intesities set to 1 */
    glEnable(GL_NORMALIZE);

    /* Transparency settings */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

static void other_initialize()
{
    eye_x = 0;
    eye_y = 8*CUBE_SIZE;
    eye_z = 0;
    to_x = 5*CUBE_SIZE;
    to_y = 2*CUBE_SIZE;
    to_z = -5*CUBE_SIZE;
    n_x = 0;
    n_y = 1;
    n_z = 0;

    FILE* f = fopen(map_dimensions_file, "r");
    osAssert(f != NULL, "Error opening file \"map_dimensions.txt\"\n");

    fscanf(f, "%d %d", &map_rows, &map_cols);
    fclose(f);

    srand(time(NULL));
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
            /* Color is initially the same as type - works for teleport colors */
            map[i][j].color = map[i][j].type;
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

static void set_coeffs(float r, float g, float b, float a)
{
    coeffs[0] = r;
    coeffs[1] = g;
    coeffs[2] = b;
    coeffs[3] = a;
}

static void draw_axis()
{
    glDisable(GL_LIGHTING);

    glBegin(GL_LINES);
        glColor3f(1, 0, 0);
        glVertex3f(0, 0, 0);
        glVertex3f(15*CUBE_SIZE, 0, 0);

        glColor3f(0, 1, 0);
        glVertex3f(0, 0, 0);
        glVertex3f(0, 15*CUBE_SIZE, 0);

        glColor3f(0, 0, 1);
        glVertex3f(0, 0, 0);
        glVertex3f(0, 0, -15*CUBE_SIZE);
    glEnd();

    glEnable(GL_LIGHTING);
}

static void set_norm_vert_cylinder(float r, float phi, float h)
{
    glNormal3f(r*sin(phi), h, r*cos(phi));
    glVertex3f(r*sin(phi), h, r*cos(phi));
}

static void draw_cylinder(float r, float h)
{
    float phi;

    glBegin(GL_TRIANGLE_STRIP);
    for (phi = 0; phi <= 2*PI + EPS; phi += PI/20) {
        set_norm_vert_cylinder(r, phi, 0);
        set_norm_vert_cylinder(r, phi, h);
    }
    glEnd();
}

static void create_key()
{
    float body_radius = 0.015;
    float body_height = 0.2;

    /* Key torus part */
    glPushMatrix();
        glTranslatef(0.04, 0, 0);
        glutSolidTorus(0.015, 0.05, 10, 20);
    glPopMatrix();

    /* Key body */
    glPushMatrix();
        glRotatef(90, 0, 0, 1);
        draw_cylinder(body_radius, body_height);
    glPopMatrix();

    /* "Teeth" */
    glPushMatrix();
        glTranslatef(-0.16, -0.1, 0);
        draw_cylinder(body_radius/1.5, body_height/3);

        glTranslatef(0.05, 0, 0);
        draw_cylinder(body_radius/1.5, body_height/3);
    glPopMatrix();
}

static void create_teleport(float x, float y, float z, char color)
{
    /* Reminder: (x,y,z) are the coordinates of the center of the cube */
    float phi;
    float r = CUBE_SIZE / 2 - 0.05;
    float interval_01;

    /* Teleport lines height will be in [a, b] interval */
    float a = 0.3, b = CUBE_SIZE;

    switch (color) {
        case 'b':   set_coeffs(0, 0, 1, 0.8); break;
        case 'r':   set_coeffs(0.9, 0.1, 0.1, 0.8); break;
        case 'g':   set_coeffs(0, 0.55, 0, 0.8); break;
        case 'y':   set_coeffs(1, 0.85, 0, 0.8); break;
        case 'o':   set_coeffs(1, 0.55, 0, 0.8); break;
        case 'm':   set_coeffs(1, 0.4, 0.75, 0.8); break;
        case 'p':   set_coeffs(0.3, 0, 0.6, 0.8); break;
        case 'c':   set_coeffs(0, 0.75, 1, 0.8); break;
        default:    set_coeffs(1, 1, 1, 0.8); break;
    }

    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, coeffs);

    /* glutTimerFunc(10, on_timer, TELEPORT_TIMER_ID); */

    /* Teleport outer circle: has slightly lower alpha (is transparent) */
    glBegin(GL_TRIANGLE_FAN);

        glNormal3f(0, 1, 0);
        glVertex3f(x, y, z);

        for (phi = 0; phi <= 2*PI + EPS; phi += PI / 20) {
            glVertex3f(x + r * cos(phi), y, z + r * sin(phi));
        }
    glEnd();

    /* Teleport inner circle */
    coeffs[3] = 0.9;
    glBegin(GL_TRIANGLE_FAN);

        glNormal3f(0, 1, 0);
        glVertex3f(x, y + EPS/2, z);

        for (phi = 0; phi <= 2*PI + EPS; phi += PI / 20) {
            glVertex3f(x + r/1.5 * cos(phi), y + EPS/2, z + r/1.5 * sin(phi));
        }
    glEnd();

    coeffs[3] = 0.8;

    glLineWidth(1.8);
    for (phi = 0; phi <= 2*PI + EPS; phi += PI / 20) {
        interval_01 = rand() / (float) RAND_MAX;

        glBegin(GL_LINES);
            glVertex3f(x  + r * cos(phi * 1.2 /* + teleport_time */), 
                       y, 
                       z + r * sin(phi * 1.2 /* + teleport_time */));
            glVertex3f(x + r * cos(phi * 1.2 /* + teleport_time */), 
                       y + (interval_01 * (b-a) + a), 
                       z + r * sin(phi * 1.2 /* + teleport_time */));
        glEnd();        
    }
}

static void create_map()
{
    int i, j;

    float switch_height = 0.4;
    float elevator_height = 0.3;
   
    glPushMatrix();

        glTranslatef(CUBE_SIZE / 2, - CUBE_SIZE / 2, - CUBE_SIZE / 2);
        for (i = map_rows - 1; i >= 0; i--) {
            for (j = 0; j < map_cols; j++) {

                /* x and z coordinates of the CENTER of the cube */
                float x = j*CUBE_SIZE;
                float z = -(map_rows - 1 - i)*CUBE_SIZE;

                switch (map[i][j].type) {
                    /* Wall case */
                    case 'w':
                        glPushMatrix();
                            glTranslatef(x, 0, z);
                            set_coeffs(0.2, 0.7, 0.1, 1);
                            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, coeffs);
                            glutSolidCube(CUBE_SIZE);

                        if (map[i][j].height != 0) {
                            glTranslatef(0, CUBE_SIZE, 0);
                            glScalef(1, map[i][j].height, 1);
                            set_coeffs(0.7, 0.5, 0.2, 1);                           
                            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, coeffs);
                            glutSolidCube(CUBE_SIZE);
                        }

                        glPopMatrix();
                        break;

                    /* Lava case */
                    case 'l':
                        glPushMatrix();
                            glTranslatef(x, 0, z);
                            set_coeffs(0.9, 0.2, 0.1, 1);
                            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, coeffs);
                            glutSolidCube(CUBE_SIZE);
                        glPopMatrix();
                        break;

                    /* Door case */
                    case 'd':
                        glPushMatrix();
                            glTranslatef(x, 0, z);
                            set_coeffs(0.2, 0.7, 0.1, 1);
                            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, coeffs);
                            glutSolidCube(CUBE_SIZE);
                        glPopMatrix();

                        if (map[i][j].height == 0) {
                            glPushMatrix();
                                glTranslatef(x, CUBE_SIZE, z);
                                set_coeffs(0.5, 0.2, 0.1, 1);
                                glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, coeffs);
                                glutSolidCube(CUBE_SIZE);
                            glPopMatrix();
                        } else {
                            glPushMatrix();
                                glTranslatef(x, CUBE_SIZE, z);
                                glScalef(1, map[i][j].height, 1);
                                set_coeffs(0.5, 0.6, 0.1, 1);
                                glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, coeffs);
                                glutSolidCube(CUBE_SIZE);
                            glPopMatrix();

                            glPushMatrix();
                                glTranslatef(x, map[i][j].height * CUBE_SIZE, z);
                                set_coeffs(0.5, 0.2, 0.1, 1);
                                glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, coeffs);
                                glutSolidCube(CUBE_SIZE);
                            glPopMatrix();
                        }

                        break;

                    /* Elevator case */
                    case 'e':
                        glPushMatrix();
                            glTranslatef(x, 0, z);
                            set_coeffs(0.2, 0.7, 0.1, 1);
                            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, coeffs);
                            glutSolidCube(CUBE_SIZE);
                        glPopMatrix();

                        if (map[i][j].height == 0) {
                            glPushMatrix();
                                glTranslatef(x, CUBE_SIZE - elevator_height, z);
                                glScalef(1, elevator_height, 1);
                                set_coeffs(0.7, 0.7, 0.4, 0.9);
                                glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, coeffs);
                                glutSolidCube(CUBE_SIZE);
                            glPopMatrix();
                        } else {
                            glPushMatrix();
                                glTranslatef(x, CUBE_SIZE, z);
                                glScalef(1, map[i][j].height, 1);
                                set_coeffs(0.7, 0.5, 0.2, 1);                               
                                glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, coeffs);
                                glutSolidCube(CUBE_SIZE);
                            glPopMatrix();

                            glPushMatrix();
                                glTranslatef(x, map[i][j].height * CUBE_SIZE, z);
                                glScalef(1, 0.3, 1);
                                set_coeffs(0.7, 0.7, 0.4, 0.9);
                                glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, coeffs);
                                glutSolidCube(CUBE_SIZE);
                            glPopMatrix();
                        }

                        break;

                    /* Key case */
                    case 'k':
                        glPushMatrix();
                            glTranslatef(x, 0, z);
                            set_coeffs(0.2, 0.7, 0.1, 1);
                            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, coeffs);
                            glutSolidCube(CUBE_SIZE);
                        glPopMatrix();

                        if (map[i][j].height == 0) {
                            glPushMatrix();
                                glTranslatef(x, CUBE_SIZE + 5*EPS, z);
                                set_coeffs(0.8, 0.8, 0, 1);
                                glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, coeffs);
                                create_key();
                            glPopMatrix();
                        } else {
                            glPushMatrix();
                                glTranslatef(x, CUBE_SIZE, z);
                                glScalef(1, map[i][j].height, 1);
                                set_coeffs(0.7, 0.5, 0.2, 1);                               
                                glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, coeffs);
                                glutSolidCube(CUBE_SIZE);
                            glPopMatrix();

                            glPushMatrix();
                                glTranslatef(x, map[i][j].height * CUBE_SIZE + 5*EPS, z);
                                set_coeffs(0.8, 0.8, 0, 1);
                                glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, coeffs);
                                create_key();
                            glPopMatrix();
                        }

                        break;

                    /* Switch case */
                    case 's':
                        glPushMatrix();
                            glTranslatef(x, 0, z);
                            set_coeffs(0.2, 0.7, 0.1, 1);
                            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, coeffs);
                            glutSolidCube(CUBE_SIZE);
                        glPopMatrix();

                        if (map[i][j].height == 0) {
                            glPushMatrix();
                                glTranslatef(x, CUBE_SIZE, z);
                                set_coeffs(0.5, 0.5, 0.7, 1);
                                glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, coeffs);
                                glRotatef(30, 0, 0, 1);
                                draw_cylinder(0.035, switch_height);
                            glPopMatrix();
                        } else {
                            glPushMatrix();
                                glTranslatef(x, CUBE_SIZE, z);
                                glScalef(1, map[i][j].height, 1);
                                set_coeffs(0.7, 0.5, 0.2, 1);                               
                                glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, coeffs);
                                glutSolidCube(CUBE_SIZE);
                            glPopMatrix();

                            glPushMatrix();
                                glTranslatef(x, map[i][j].height * CUBE_SIZE, z);
                                set_coeffs(0.5, 0.5, 0.7, 1);
                                glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, coeffs);
                                glRotatef(-30, 0, 0, 1);
                                draw_cylinder(0.035, switch_height);
                            glPopMatrix();
                        }

                        break;

                    /* Teleport case */
                    default:
                        glPushMatrix();
                            glTranslatef(x, 0, z);
                            set_coeffs(0.2, 0.7, 0.1, 1);
                            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, coeffs);
                            glutSolidCube(CUBE_SIZE);
                        glPopMatrix();

                        if (map[i][j].height == 0) {
                            glPushMatrix();
                                glTranslatef(x, 0, z);
                                create_teleport(0, CUBE_SIZE/2 + EPS, 0, map[i][j].color);
                            glPopMatrix();
                        } else if (map[i][j].height == 1) {
                            glPushMatrix();
                                glTranslatef(x, CUBE_SIZE, z);
                                set_coeffs(0.7, 0.5, 0.2, 1);                           
                                glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, coeffs);
                                glutSolidCube(CUBE_SIZE);
                                create_teleport(0, CUBE_SIZE/2 + EPS, 0, map[i][j].color);
                            glPopMatrix();
                        } else {
                            glPushMatrix();
                                glTranslatef(x, CUBE_SIZE, z);
                                glScalef(1, map[i][j].height, 1);
                                set_coeffs(0.7, 0.5, 0.2, 1);                           
                                glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, coeffs);
                                glutSolidCube(CUBE_SIZE);
                            glPopMatrix();

                            glPushMatrix();
                                glTranslatef(x, map[i][j].height * CUBE_SIZE, z);
                                create_teleport(0, EPS, 0, map[i][j].color);
                            glPopMatrix();
                        }

                        break;
                }
            }
        }

    glPopMatrix();

}
/*
static void reset_material()
{
    GLfloat ambient_coeffs[] = {0.2, 0.2, 0.2, 1};
    GLfloat diffuse_coeffs[] = {0.5, 0.5, 0.5, 1};
    GLfloat specular_coeffs[] = {0.3, 0.3, 0.3, 1};
    GLfloat shininess = 30;
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient_coeffs);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse_coeffs);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular_coeffs);
    glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
}
*/

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
              n_x, n_y, n_z);

    draw_axis();

    create_map();

    glutSwapBuffers();
}