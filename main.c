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

#define SWITCH_TIMER_ID 50
#define KEY_TIMER_ID 60
#define GLOBAL_TIMER_ID 0
#define TELEPORT_TIMER_ID 1

/* Error-checking function. Used for technical C details */
#define osAssert(condition, msg) osError(condition, msg)
void osError(bool condition, const char* msg) {
    if (!condition) {
        perror(msg);
        exit(EXIT_FAILURE);
    }
}



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
static bool global_timer_active = true;
static float global_time_parameter = 0;

/*Basic glut callback functions declarations*/
static void on_keyboard(unsigned char key, int x, int y);
static void on_timer(int value);
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

/* Support function that sets diffuse coeffs in a global vector */
static void set_diffuse(float r, float g, float b, float a);

/* Support function that resets default material lighting settings */
/* static void reset_material(); */

/* Support function that draws coordinate system */
static void draw_axis();

/* Support functions that draw cylinder */
static void set_norm_vert_cylinder(float r, float phi, float h);
static void draw_cylinder(float r, float h);

/* Creates wall with the given cube size and height.
 * Wall is constructed of height cubes piled on each other. */
static void create_wall(float cube_size, int height);

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

    glutTimerFunc(20, on_timer, GLOBAL_TIMER_ID);

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

        case 'z':
        case 'Z':
            if (!global_timer_active) {
                global_timer_active = true;
                glutTimerFunc(20, on_timer, GLOBAL_TIMER_ID);
            }
            break;

        case 'x':
        case 'X':
            global_time_parameter = false;
            glutPostRedisplay();
            break;

        case 'r':
        case 'R':
            /* Also reseting camera position */
            eye_x = 0;
            eye_y = 8*CUBE_SIZE;
            eye_z = 0;

            global_time_parameter = 0;
            glutPostRedisplay();
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
    }
}

static void on_timer(int value)
{
    if (value != GLOBAL_TIMER_ID) {
        return;
    }

    global_time_parameter += 1;

    glutPostRedisplay();

    if (global_time_parameter) {
        glutTimerFunc(20, on_timer, GLOBAL_TIMER_ID);
    }
}

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

static void set_diffuse(float r, float g, float b, float a)
{
    coeffs[0] = r;
    coeffs[1] = g;
    coeffs[2] = b;
    coeffs[3] = a;
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, coeffs);
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

static void create_wall(float cube_size, int height)
{
    int i;

    glPushMatrix();
        glTranslatef(0, -cube_size, 0);
        for (i = 1; i <= height; i++) {
            glTranslatef(0, cube_size, 0);
            glutSolidCube(cube_size);
        }
    glPopMatrix();
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

void set_vector3f(GLfloat* vector, float r, float g, float b) {
    vector[0] = r;
    vector[1] = g;
    vector[2] = b;
}

static void create_teleport(float x, float y, float z, char color)
{
    /* Reminder: (x,y,z) are the coordinates of the center of the cube */
    float phi;
    float r = CUBE_SIZE / 2 - 0.05;

    GLfloat inner[3];
    GLfloat outer[3];

    switch (color) {
        case 'b':   
            set_vector3f(inner, 0, 0.3, 1);             
            set_vector3f(outer, 0, 0.15, 0.9); 
            break;
        case 'r':   
            set_vector3f(inner, 1, 0.1, 0.1);
            set_vector3f(outer, 0.85, 0, 0);
            break;
        case 'g':   
            set_vector3f(inner, 0.1, 0.7, 0.1);
            set_vector3f(outer, 0, 0.55, 0);
            break;
        case 'y':   
            set_vector3f(inner, 0.9, 0.55, 0);
            set_vector3f(outer, 1, 0.85, 0.1);
            break;
        case 'o':   
            set_vector3f(inner, 1, 0.6, 0.1);
            set_vector3f(outer, 0.9, 0.4, 0);
            break;
        case 'm':   
            set_vector3f(inner, 0.85, 0.3, 0.6); 
            set_vector3f(outer, 1, 0.4, 0.75);
            break;
        case 'p':   
            set_vector3f(inner, 0.4, 0.1, 0.7);
            set_vector3f(outer, 0.3, 0, 0.55);
            break;
        case 'c':   
            set_vector3f(inner, 0, 0.5, 0.85); 
            set_vector3f(outer, 0.1, 0.75, 1); 
            break;
        default:    
            set_vector3f(inner, 1, 1, 1); 
            set_vector3f(outer, 0.9, 0.9, 0.9); 
            break;
    }

    glDisable(GL_LIGHTING);

    glBegin(GL_TRIANGLE_FAN);

        glColor3fv(inner);
        glVertex3f(x, y, z);

        for (phi = 0; phi <= 2*PI + EPS; phi += PI / 20) {
            glColor3fv(outer);
            glVertex3f(x + r * cos(phi), y, z + r * sin(phi));
        }
    glEnd();

    glLineWidth(1.8);
    glColor3fv(outer);

    for (phi = 0; phi <= 2*PI + EPS; phi += PI / 20) {
        glBegin(GL_LINES);
            glVertex3f(x  + r * cos(phi * 1.2 /* + teleport_time */), 
                       y, 
                       z + r * sin(phi * 1.2 /* + teleport_time */));
            glVertex3f(x + r * cos(phi * 1.2 /* + teleport_time */), 
                       y + CUBE_SIZE/1.3, 
                       z + r * sin(phi * 1.2 /* + teleport_time */));
        glEnd();        
    }

    glEnable(GL_LIGHTING);
}

static void create_map()
{
    int i, j;

    float switch_height = 0.4;
    float elevator_height = 0.2;

    /* Special factor that fixes the elevator position since scaling
     * will cause the elevator to float in space */
    float elevator_scale_move_factor = 0.25;
   
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
                        /* Grass */
                        glPushMatrix();
                            glTranslatef(x, 0, z);
                            set_diffuse(0.2, 0.7, 0.1, 1);
                            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, coeffs);
                            glutSolidCube(CUBE_SIZE);

                        /* Wall */
                        if (map[i][j].height != 0) {
                            glTranslatef(0, CUBE_SIZE, 0);
                            set_diffuse(0.7, 0.5, 0.2, 1);                           
                            create_wall(CUBE_SIZE, map[i][j].height);
                        }

                        glPopMatrix();
                        break;

                    /* Lava case - always with 0 height*/
                    case 'l':
                        glPushMatrix();
                            glTranslatef(x, 0, z);
                            set_diffuse(0.9, 0.2, 0.1, 1);
                            glutSolidCube(CUBE_SIZE);
                        glPopMatrix();
                        break;

                    /* Door case */
                    case 'd':
                        /* Grass and wall */
                        glPushMatrix();
                            glTranslatef(x, 0, z);
                            set_diffuse(0.2, 0.7, 0.1, 1);
                            glutSolidCube(CUBE_SIZE);
                        
                            glTranslatef(0, CUBE_SIZE, 0);
                            set_diffuse(0.7, 0.5, 0.2, 1);                           
                            create_wall(CUBE_SIZE, map[i][j].height - 1);
                        glPopMatrix();

                        /* Door */
                        glPushMatrix();
                            glTranslatef(x, map[i][j].height*CUBE_SIZE, z);
                            set_diffuse(0.5, 0.2, 0.1, 1);
                            glutSolidCube(CUBE_SIZE);
                        glPopMatrix();
                        break;

                    /* Elevator case */
                    case 'e':
                        /* Grass and wall */
                        glPushMatrix();
                            glTranslatef(x, 0, z);
                            set_diffuse(0.2, 0.7, 0.1, 1);
                            glutSolidCube(CUBE_SIZE);
                        
                            glTranslatef(0, CUBE_SIZE, 0);
                            set_diffuse(0.7, 0.5, 0.2, 1);                           
                            create_wall(CUBE_SIZE, map[i][j].height - 1);
                        glPopMatrix();

                        /* Elevator */
                        glPushMatrix();
                            glTranslatef(x, map[i][j].height*CUBE_SIZE, z);
                            glTranslatef(0, -elevator_scale_move_factor, 0);
                            glScalef(1, elevator_height, 1);
                            set_diffuse(0.7, 0.7, 0.4, 1);
                            glutSolidCube(CUBE_SIZE);
                        glPopMatrix();
                        break;

                    /* Key case */
                    case 'k':
                        /* Grass and wall */
                        glPushMatrix();
                            glTranslatef(x, 0, z);
                            set_diffuse(0.2, 0.7, 0.1, 1);
                            glutSolidCube(CUBE_SIZE);
                        
                            glTranslatef(0, CUBE_SIZE, 0);
                            set_diffuse(0.7, 0.5, 0.2, 1);                           
                            create_wall(CUBE_SIZE, map[i][j].height - 1);
                        glPopMatrix();

                        /* Key */
                        glPushMatrix();
                            glTranslatef(x, map[i][j].height*CUBE_SIZE, z);
                            set_diffuse(0.8, 0.8, 0, 1);

                            glTranslatef(0, CUBE_SIZE/5 * sin(2 * global_time_parameter * DEG_TO_RAD), 0);
                            glRotatef(-global_time_parameter * 2, 0, 1, 0);

                            create_key();
                        glPopMatrix();
                        break;

                    /* Switch case */
                    case 's':
                        /* Grass and wall */
                        glPushMatrix();
                            glTranslatef(x, 0, z);
                            set_diffuse(0.2, 0.7, 0.1, 1);
                            glutSolidCube(CUBE_SIZE);
                        
                            glTranslatef(0, CUBE_SIZE, 0);
                            set_diffuse(0.7, 0.5, 0.2, 1);                           
                            create_wall(CUBE_SIZE, map[i][j].height - 1);
                        glPopMatrix();

                        /* Switch */
                        glPushMatrix();
                            glTranslatef(x, map[i][j].height*CUBE_SIZE, z);

                            /* glRotatef(-30, 0, 0, 1) will slightly move the switch
                             * to the right; we fix it by moving slightly to the left.
                             * Also, -CUBE_SIZE/2 is to lower the floating effect */
                            glTranslatef(0, -CUBE_SIZE/2, 0);

                            glRotatef(global_time_parameter * 2, 0, 1, 0);

                            glRotatef(-25, 0, 0, 1);
                            set_diffuse(0.5, 0.5, 0.7, 1);
                            draw_cylinder(0.035, switch_height);
                        glPopMatrix();
                        break;

                    /* Teleport case */
                    default:
                        /* Grass and wall */
                        glPushMatrix();
                            glTranslatef(x, 0, z);
                            set_diffuse(0.2, 0.7, 0.1, 1);
                            glutSolidCube(CUBE_SIZE);
                        
                            glTranslatef(0, CUBE_SIZE, 0);
                            set_diffuse(0.7, 0.5, 0.2, 1);                           
                            create_wall(CUBE_SIZE, map[i][j].height - 1);
                        glPopMatrix();

                        /* Teleport */
                        glPushMatrix();
                            glTranslatef(x, map[i][j].height*CUBE_SIZE, z);

                            /* Teleport floating effect fix */
                            glTranslatef(0, -CUBE_SIZE/2 + EPS, 0);

                            create_teleport(0, 0, 0, map[i][j].color);
                        glPopMatrix();
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