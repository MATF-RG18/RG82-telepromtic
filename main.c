#include <GL/glut.h>
#include <cglm/cglm.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>

/* Error-checking function. Used for technical C details */
#define osAssert(condition, msg) osError(condition, msg)
void osError(bool condition, const char* msg) {
    if (!condition) {
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

#define EXIT_KEY 27

#define MAX_FILE_NAME 32

#define PI 3.14159265359
#define EPS 0.01
#define RAD_TO_DEG 180/PI
#define DEG_TO_RAD PI/180

#define GLOBAL_TIMER_ID 0
#define TIMER_INTERVAL 20

#define TELEPORT_TIMER_ID 1

#define ELEVATOR_TIMER_ID_12 12
#define ELEVATOR_TIMER_ID_58 58
#define ELEVATOR_TIMER_ID_89 89
#define ELEVATOR_TIMER_ID_25 25

#define DOOR_TIMER_ID_18 18
#define DOOR_TIMER_ID_27 27
#define DOOR_TIMER_ID_41 41
#define DOOR_TIMER_ID_86 86

/* Structure that will keep data for every field cube. 
 * 1) type can be: 'w' - wall, 'l' - lava, 'd' - door, 'e' - elevator,
 *    'k' - key, 's' - switch, 'X' - goal, '@' - player starting position
 * 2) color can be: 'r' - red, 'g' - green, 'b' - blue, 'y' - yellow, 'o' - orange,
 *    'p' - purple, 'c' - cyan, 'm' - magenta. In case of teleports this is also the type
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

/* Every part of the field is made of cube of fixed size.
 * All other objects' size are relative to this size */
const static float CUBE_SIZE = 3.6;

/* Material component coeffs that will be updated by support function */
static GLfloat coeffs[] = {0, 0, 0, 1};

/* Main game matrix that will store basic info about every game cube */
static FieldData** map = NULL;

/* Global timer flag and parameter: global timer is always active */
static bool global_timer_active = true;
static float global_time_parameter = 0;

/* Teleport timer flag and parameter: used for teleport animation */
static float teleport_parameter = 0;
static bool teleport_timer_active = true;

/* Switch/Elevator flags and parameters: they are connected respectively */
static bool has_switch_98 = false;
static float elevator_parameter_12 = 0;
static bool elevator_timer_12_active = false;

static bool has_switch_44 = false;
static float elevator_parameter_58 = 0;
static bool elevator_timer_58_active = false;

static bool has_switch_73 = false;
static float elevator_parameter_89 = 0;
static bool elevator_timer_89_active = false;

static bool has_switch_77 = false;
static float elevator_parameter_25 = 0;
static bool elevator_timer_25_active = false;

/* Key/Door flags and parameters: they are connected respectively */
static bool has_key_99 = false;
static float door_parameter_18 = 0;
static bool door_timer_18_active = false;

static bool has_key_23 = false;
static float door_parameter_27 = 0;
static bool door_timer_27_active = false;

static bool has_key_11 = false;
static float door_parameter_41 = 0;
static bool door_timer_41_active = false;

static bool has_key_71 = false;
static float door_parameter_86 = 0;
static bool door_timer_86_active = false;

/* Camera position, target and up vectors */
static vec3 camera_pos = (vec3){0.0f, 0.0f, 3.0f};
static vec3 camera_front = (vec3){0.0f, 0.0f, -1.0f};
static vec3 camera_up = (vec3){0.0f, 1.0f, 0.0f};

/* Camera direction and right vectors */
static vec3 camera_direction;
static vec3 camera_right;

/* Camera speed (player movement speed) */
static float camera_speed = 0.2f;

/* Last (x, y) coordinates of mouse pointer registered on window */
static float last_x = 400;
static float last_y = 300;

/* Center coordinates */
static int center_x = 400;
static int center_y = 300;

/* Flag - false after mouse is catched for the first time */
static bool first_mouse = true;

/* Pitch and Yaw angles used for camera rotation */
static float theta = 0; /* [-89, 89] deg */
static float phi = 0;   /* [0, 180) deg */

/*Basic glut callback functions declarations*/
static void on_keyboard(unsigned char key, int x, int y);
static void on_mouse_passive(int x, int y);
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

/* Support function that draws coordinate system */
static void draw_axis();

/* Support functions that draw cylinder */
static void set_norm_vert_cylinder(float r, float phi, float h);
static void draw_cylinder(float r, float h);

/* Creates wall with the given cube size and height.
 * Wall is constructed of height cubes piled on each other. */
static void create_wall(float cube_size, int height);

/* Support function used for coloring */
static void set_vector4f(GLfloat* vector, float r, float g, float b, float a); 

/* If proper switch is gathered, it's not rendered on the map */
static bool check_switch_inventory(int i, int j);

/* If proper key is gathered, it's not rendered on the map */
static bool check_key_inventory(int i, int j);

/* If door has moved beyond minimal point, it's no longer rendered on the map */
static bool check_door_moved(int i, int j);

/* Support function that checks the player position */
static bool check_height(float min_height, float max_height); 

/* Creates key of fixed size */
static void create_key();

/* Creates switch of fixed size */
static void create_switch();

/* Creates teleport with the given color */
static void create_teleport(float x, float y, float z, char color);

/* Moves elevators if their connected switches are gathered */
static void move_elevator(int i, int j, float e_height);

/* Moves doors if their connected keys are gathered */
static void move_door(int i, int j);

/* Checking player position upon camera_pos changes: 
 * function checks map matrix type and renders proper effect */
static void check_player_position();

/* Support function that checks if player is positioned inside teleport circle */
static bool check_inside_circle(int i, int j);

/* Function that handles teleportation if player is on right position
 * and activates the teleport */
static void check_teleportation();


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
    glutPassiveMotionFunc(on_mouse_passive);
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

    glutTimerFunc(TIMER_INTERVAL, on_timer, GLOBAL_TIMER_ID);
    glutTimerFunc(TIMER_INTERVAL, on_timer, TELEPORT_TIMER_ID);

    /* Entering OpenGL main loop */
    glutMainLoop();

    /* Freeing dynamicly allocated space for map */
    map = free_map(map);

    return 0;
}

static void on_keyboard(unsigned char key, int x, int y)
{
    if (key == EXIT_KEY) {
        /* Exiting program */
        exit(EXIT_SUCCESS);
    } else if (key == '1') {
        has_switch_98 = true;
        glutPostRedisplay();
    } else if (key == '2') {
        has_switch_44 = true;
        glutPostRedisplay();
    } else if (key == '3') {
        has_switch_73 = true;
        glutPostRedisplay();
    } else if (key == '4') {
        has_switch_77 = true;
        glutPostRedisplay();
    } else if (key == '5') {
        has_key_11 = true;
        glutPostRedisplay();
    } else if (key == '6') {
        has_key_23 = true;
        glutPostRedisplay();
    } else if (key == '7') {
        has_key_71 = true;
        glutPostRedisplay();
    } else if (key == '8') {
        has_key_99 = true;
        glutPostRedisplay();
    } else if (key == 'r' || key == 'R') {
        /* Reseting all parameters */
        global_time_parameter = 0;

        elevator_parameter_12 = 0;
        elevator_parameter_25 = 0;
        elevator_parameter_58 = 0;
        elevator_parameter_89 = 0;

        elevator_timer_12_active = false;
        elevator_timer_25_active = false;
        elevator_timer_58_active = false;
        elevator_timer_89_active = false;

        has_switch_44 = false;
        has_switch_73 = false;
        has_switch_77 = false;
        has_switch_98 = false;

        door_parameter_18 = 0;
        door_parameter_27 = 0;
        door_parameter_41 = 0;
        door_parameter_86 = 0;

        door_timer_18_active = false;
        door_timer_27_active = false;
        door_timer_41_active = false;
        door_timer_86_active = false;

        has_key_11 = false;
        has_key_23 = false;
        has_key_71 = false;
        has_key_99 = false;

        glutPostRedisplay();
    } else if (key == 't' || key == 'T') {
        /* Teleportation if player is in proper position */
        check_teleportation();
        glutPostRedisplay();
    } else if (key == 'w' || key == 'W') {
        /* Moving forward */
        glm_vec3_muladds(camera_front, camera_speed, camera_pos);
        glutPostRedisplay();
        check_player_position();
    } else if (key == 's' || key == 'S') {
        /* Moving backward */
        glm_vec3_muladds(camera_front, -camera_speed, camera_pos);
        glutPostRedisplay();
        check_player_position();
    } else if (key == 'a' || key == 'A') {
        /* Moving left */
        glm_vec3_muladds(camera_right, -camera_speed, camera_pos);
        glutPostRedisplay();
        check_player_position();
    } else if (key == 'd' || key == 'D') {
        /* Moving right */
        glm_vec3_muladds(camera_right, camera_speed, camera_pos);
        glutPostRedisplay();
        check_player_position();
    }

}

static void on_mouse_passive(int x, int y)
{
    /* NOTE: code taken from https://learnopengl.com/Getting-started/Camera */

    /* First mouse register */
    if (first_mouse) {
        last_x = x;
        last_y = y;
        first_mouse = false;

        glutWarpPointer(center_x, center_y);
    }

    /* Calculating mouse move */
    float x_offset = x - last_x;
    float y_offset = last_y - y;
    last_x = x;
    last_y = y;

    float sensitivity = 0.5f;
    x_offset *= sensitivity;
    y_offset *= sensitivity;

    /* Calculating Euler yaw (phi) and pitch (theta) angles */
    phi += x_offset;
    theta += y_offset;

    /* Fixing camera rotation to 'sky' and 'floor' */
    if (theta >= 89) {
        theta = 89;
    }
    if (theta <= -89) {
        theta = -89;
    }

    /* Calculating camera direction */
    vec3 front;
    float front_x = cos(phi * DEG_TO_RAD) * cos(theta * DEG_TO_RAD);
    float front_y = sin(theta * DEG_TO_RAD);
    float front_z = sin(phi * DEG_TO_RAD) * cos(theta * DEG_TO_RAD);

    glm_vec3((vec3){front_x, front_y, front_z}, front);
    glm_normalize(front);

    glm_vec3_copy(front, camera_front);
}

static void on_timer(int value)
{
    if (value == GLOBAL_TIMER_ID) {

        global_time_parameter += 1;

        glutPostRedisplay();

        if (global_timer_active) {
            glutTimerFunc(TIMER_INTERVAL, on_timer, GLOBAL_TIMER_ID);
        }
    } else if (value == TELEPORT_TIMER_ID) {

        teleport_parameter += PI/90;

        glutPostRedisplay();

        if (teleport_timer_active) {
            glutTimerFunc(TIMER_INTERVAL, on_timer, TELEPORT_TIMER_ID);
        }
    } else if (value == ELEVATOR_TIMER_ID_12) {

        elevator_parameter_12 += PI/180;

        glutPostRedisplay();

        if (elevator_timer_12_active) {
            glutTimerFunc(TIMER_INTERVAL, on_timer, ELEVATOR_TIMER_ID_12);
        }
    } else if (value == ELEVATOR_TIMER_ID_58) {

        elevator_parameter_58 += PI/180;

        glutPostRedisplay();

        if (elevator_timer_58_active) {
            glutTimerFunc(TIMER_INTERVAL, on_timer, ELEVATOR_TIMER_ID_58);
        }
    } else if (value == ELEVATOR_TIMER_ID_89) {

        elevator_parameter_89 += PI/180;

        glutPostRedisplay();

        if (elevator_timer_89_active) {
            glutTimerFunc(TIMER_INTERVAL, on_timer, ELEVATOR_TIMER_ID_89);
        }
    } else if (value == ELEVATOR_TIMER_ID_25) {

        elevator_parameter_25 += PI/180;

        glutPostRedisplay();

        if (elevator_timer_25_active) {
            glutTimerFunc(TIMER_INTERVAL, on_timer, ELEVATOR_TIMER_ID_25);
        }
    } else if (value == DOOR_TIMER_ID_18) {

        door_parameter_18 += CUBE_SIZE / 60;
        if (door_parameter_18 >= CUBE_SIZE + 0.1) {
            door_parameter_18 = -1;
            door_timer_18_active = false;
        }

        glutPostRedisplay();

        if (door_timer_18_active) {
            glutTimerFunc(TIMER_INTERVAL, on_timer, DOOR_TIMER_ID_18);
        }
    } else if (value == DOOR_TIMER_ID_27) {

        door_parameter_27 += CUBE_SIZE / 60;
        if (door_parameter_27 >= CUBE_SIZE + 0.1) {
            door_parameter_27 = -1;
            door_timer_27_active = false;
        }

        glutPostRedisplay();

        if (door_timer_27_active) {
            glutTimerFunc(TIMER_INTERVAL, on_timer, DOOR_TIMER_ID_27);
        }
    } else if (value == DOOR_TIMER_ID_41) {

        door_parameter_41 += CUBE_SIZE / 60;
        if (door_parameter_41 >= CUBE_SIZE + 0.1) {
            door_parameter_41 = -1;
            door_timer_41_active = false;
        }

        glutPostRedisplay();

        if (door_timer_41_active) {
            glutTimerFunc(TIMER_INTERVAL, on_timer, DOOR_TIMER_ID_41);
        }
    } else if (value == DOOR_TIMER_ID_86) {

        door_parameter_86 += CUBE_SIZE / 60;
        if (door_parameter_86 >= CUBE_SIZE + 0.1) {
            door_parameter_86 = -1;
            door_timer_86_active = false;
        }

        glutPostRedisplay();

        if (door_timer_86_active) {
            glutTimerFunc(TIMER_INTERVAL, on_timer, DOOR_TIMER_ID_86);
        }
    } else {
        return;
    }
}

static void on_reshape(int width, int height)
{
    /* Setting view port and view perspective */
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, (float)width / height, 1, 20*CUBE_SIZE);
}

static void glut_initialize()
{   
    glClearColor(0.7, 0.7, 0.7, 0);
    glEnable(GL_DEPTH_TEST);

    /* Normal vector intesities set to 1 */
    glEnable(GL_NORMALIZE);

    /* Removes cursor visibility */
    glutSetCursor(GLUT_CURSOR_NONE);

    /* Transparency settings */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

static void other_initialize()
{
    /* Setting up camera position and target */
    glm_vec3((vec3){0.0f, 8*CUBE_SIZE, 0.0f}, camera_pos);
    glm_vec3((vec3){1.0f, 0.0f, -1.0f}, camera_front);

    /* Scanning map dimensions */
    FILE* f = fopen(map_dimensions_file, "r");
    osAssert(f != NULL, "Error opening file \"map_dimensions.txt\"\n");

    fscanf(f, "%d %d", &map_rows, &map_cols);
    fclose(f);

    /* Seeding time */
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
            /* Connection coords are initially 0: they will be updated later */
            map[i][j].to_row = map[i][j].to_col = 0;
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

        /* Connecting teleports, key/doors and switch/elevators */
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
    float body_radius = CUBE_SIZE / 40;
    float body_height = CUBE_SIZE / 3;

    /* Key torus part */
    glPushMatrix();
        glTranslatef(CUBE_SIZE / 15, 0, 0);
        glutSolidTorus(body_radius, CUBE_SIZE / 12, 10, 20);
    glPopMatrix();

    /* Key body */
    glPushMatrix();
        glRotatef(90, 0, 0, 1);
        draw_cylinder(body_radius, body_height);
    glPopMatrix();

    /* "Teeth" */
    glPushMatrix();
        glTranslatef(-CUBE_SIZE / 3.5, -CUBE_SIZE / 10, 0);
        draw_cylinder(body_radius/1.5, body_height/3);

        glTranslatef(CUBE_SIZE / 12, 0, 0);
        draw_cylinder(body_radius/1.5, body_height/3);
    glPopMatrix();
    
}

static void create_switch()
{
    draw_cylinder(CUBE_SIZE / 20, CUBE_SIZE / 2);
}

static void set_vector4f(GLfloat* vector, float r, float g, float b, float a) 
{
    vector[0] = r;
    vector[1] = g;
    vector[2] = b;
    vector[3] = a;
}

static void create_teleport(float x, float y, float z, char color)
{
    /* Reminder: (x,y,z) are the coordinates of the center of the cube */
    float r = 0.8 * CUBE_SIZE / 2; /* 80% of CUBE_SIZE / 2 */
    float r_in = CUBE_SIZE / 3.2;
    float line_height = 0.8 * CUBE_SIZE;
    float angle_scale = 2.1;
    float phi;

    GLfloat inner[4];
    GLfloat outer[4];
    GLfloat lines[4];

    switch (color) {
        case 'b':   
            set_vector4f(inner, 0, 0.3, 1, 1);             
            set_vector4f(outer, 0, 0.15, 0.9, 1); 
            set_vector4f(lines, 0, 0.3, 1, 0.9); 
            break;
        case 'r':   
            set_vector4f(inner, 1, 0.1, 0.1, 1);
            set_vector4f(outer, 0.85, 0, 0, 1);
            set_vector4f(lines, 1, 0.1, 0.1, 0.9);
            break;
        case 'g':   
            set_vector4f(inner, 0.1, 0.7, 0.1, 1);
            set_vector4f(outer, 0, 0.55, 0, 1);
            set_vector4f(lines, 0.1, 0.7, 0.1, 0.9);
            break;
        case 'y':   
            set_vector4f(inner, 0.9, 0.55, 0, 1);
            set_vector4f(outer, 1, 0.85, 0.1, 1);
            set_vector4f(lines, 0.9, 0.55, 0, 0.9);
            break;
        case 'o':   
            set_vector4f(inner, 1, 0.6, 0.1, 1);
            set_vector4f(outer, 0.9, 0.4, 0, 1);
            set_vector4f(lines, 1, 0.6, 0.1, 0.9);
            break;
        case 'm':   
            set_vector4f(inner, 0.85, 0.3, 0.6, 1); 
            set_vector4f(outer, 1, 0.4, 0.75, 1);
            set_vector4f(lines, 0.85, 0.3, 0.6, 0.9);
            break;
        case 'p':   
            set_vector4f(inner, 0.4, 0.1, 0.7, 1);
            set_vector4f(outer, 0.3, 0, 0.55, 1);
            set_vector4f(lines, 0.4, 0.1, 0.7, 0.9);
            break;
        case 'c':   
            set_vector4f(inner, 0, 0.5, 0.85, 1); 
            set_vector4f(outer, 0.1, 0.75, 1, 1);
            set_vector4f(lines, 0, 0.5, 0.85, 0.9);
            break;
        default:    
            set_vector4f(inner, 1, 1, 1, 1); 
            set_vector4f(outer, 0.8, 0.8, 0.8, 1);
            set_vector4f(lines, 1, 1, 1, 0.9);
            break;
    }

    glDisable(GL_LIGHTING);

    /* Floor gradiental circle */
    glBegin(GL_TRIANGLE_FAN);

        glColor4fv(inner);
        glVertex3f(x, y, z);

        for (phi = 0; phi <= 2*PI + EPS; phi += PI / 20) {
            glColor4fv(outer);
            glVertex3f(x + r * cos(phi), y, z + r * sin(phi));
        }
    glEnd();

    /* Inner rotating lines */
    glLineWidth(1.6);
    glColor4fv(lines);

    glRotatef(0.5 * teleport_parameter * RAD_TO_DEG, 0, 1, 0);
    for (phi = 0; phi <= 2*PI + EPS; phi += PI / 20) {
        glBegin(GL_LINES);
            glVertex3f(x  + r_in * sin(angle_scale*phi), 
                    y, 
                    z + r_in * cos(angle_scale*phi));
            glVertex3f(x + r_in * sin(angle_scale*phi), 
                    y + line_height, 
                    z + r_in * cos(angle_scale*phi));
        glEnd();        
    }

    /* Outer rings */
    glColor4fv(outer);

    float v;
    float ring_height = CUBE_SIZE / 24;

    glPushMatrix();
        glRotatef(-global_time_parameter, 0, 1, 0);
        for (v = ring_height; v <= line_height; v += 2*ring_height) {
            glTranslatef(0, 2*ring_height, 0);
            glTranslatef(0, 0.005 * sin(teleport_parameter), 0);
            draw_cylinder(r, ring_height);
        }
    glPopMatrix();

    glEnable(GL_LIGHTING);
}

static void move_elevator(int i, int j, float e_height)
{
    /* Amplitude - defines how far will elevator move */
    float amp = 0;
    /* Move parameter e [0, 1] and changes with time */
    float move_param = 0;

    /* Moving elevator on position (1, 2) */
    if (has_switch_98 && i == 1 && j == 2) {
        if (!elevator_timer_12_active) {
            elevator_timer_12_active = true;
            glutTimerFunc(TIMER_INTERVAL, on_timer, ELEVATOR_TIMER_ID_12);
        }

        amp = (1 - e_height + EPS) * CUBE_SIZE;
        move_param = (1 + sin(elevator_parameter_12 - PI/2)) / 2;

        glTranslatef(0, amp * move_param, 0);
    }

    /* Moving elevator on position (5, 8) */
    if (has_switch_44 && i == 5 && j == 8) {
        if (!elevator_timer_58_active) {
            elevator_timer_58_active = true;
            glutTimerFunc(TIMER_INTERVAL, on_timer, ELEVATOR_TIMER_ID_58);
        }

        amp = (1 - e_height + EPS) * CUBE_SIZE;
        move_param = (1 + sin(elevator_parameter_58 - PI/2)) / 2;

        glTranslatef(0, amp * move_param, 0);
    }

    /* Moving elevator on position (8, 9) */
    if (has_switch_73 && i == 8 && j == 9) {
        if (!elevator_timer_89_active) {
            elevator_timer_89_active = true;
            glutTimerFunc(TIMER_INTERVAL, on_timer, ELEVATOR_TIMER_ID_89);
        }

        amp = (1 - e_height + EPS) * CUBE_SIZE;
        move_param = (1 + sin(elevator_parameter_89 - PI/2)) / 2;

        glTranslatef(0, amp * move_param, 0);
    }

    /* Moving elevator on position (2, 5) */
    if (has_switch_77 && i == 2 && j == 5) {
        if (!elevator_timer_25_active) {
            elevator_timer_25_active = true;
            glutTimerFunc(TIMER_INTERVAL, on_timer, ELEVATOR_TIMER_ID_25);
        }

        amp = (2 - e_height + EPS) * CUBE_SIZE;
        move_param = (1 + sin(elevator_parameter_25 - PI/2)) / 2;

        glTranslatef(0, amp * move_param, 0);
    }
}

static bool check_switch_inventory(int i, int j)
{
    /* If the proper switch is gathered, switch won't be rendered */
    if (has_switch_44 && i == 4 && j == 4) {
        return false;
    } else if (has_switch_73 && i == 7 && j == 3) {
        return false;
    } else if (has_switch_77 && i == 7 && j == 7) {
        return false;
    } else if (has_switch_98 && i == 9 && j == 8) {
        return false;
    } else {
        // Ignore
    }

    /* Default: no switches are gathered and they are all rendered */
    return true;
}

static void move_door(int i, int j)
{
    /* Moving door on position (4, 1) */
    if (has_key_11 && i == 4 && j == 1) {
        if (!door_timer_41_active) {
            door_timer_41_active = true;
            glutTimerFunc(TIMER_INTERVAL, on_timer, DOOR_TIMER_ID_41);
        }

        glTranslatef(0, -door_parameter_41, 0);
    }
    
    /* Moving door on position (2, 7) */
    if (has_key_23 && i == 2 && j == 7) {
        if (!door_timer_27_active) {
            door_timer_27_active = true;
            glutTimerFunc(TIMER_INTERVAL, on_timer, DOOR_TIMER_ID_27);
        }

        glTranslatef(0, -door_parameter_27, 0);
    } 
    
    /* Moving door on position (8, 6) */
    if (has_key_71 && i == 8 && j == 6) {
        if (!door_timer_86_active) {
            door_timer_86_active = true;
            glutTimerFunc(TIMER_INTERVAL, on_timer, DOOR_TIMER_ID_86);
        }

        glTranslatef(0, -door_parameter_86, 0);
    } 
    
    /* Moving door on position (1, 8) */
    if (has_key_99 && i == 1 && j == 8) {
        if (!door_timer_18_active) {
            door_timer_18_active = true;
            glutTimerFunc(TIMER_INTERVAL, on_timer, DOOR_TIMER_ID_18);
        }

        glTranslatef(0, -door_parameter_18, 0);
    }
}

static bool check_key_inventory(int i, int j)
{
    /* If the proper key is gathered, key won't be rendered */
    if (has_key_11 && i == 1 && j == 1) {
        return false;
    } else if (has_key_23 && i == 2 && j == 3) {
        return false;
    } else if (has_key_71 && i == 7 && j == 1) {
        return false;
    } else if (has_key_99 && i == 9 && j == 9) {
        return false;
    } else {
        // Ignore
    }

    /* Default: no keys are gathered and they are all rendered */
    return true;
}

static bool check_door_moved(int i, int j)
{
    /* If door was moved, parameter will be -1 and doors won't be rendered */
    if (door_parameter_18 < 0 && i == 1 && j == 8) {
        return true;
    } else if (door_parameter_27 < 0 && i == 2 && j == 7) {
        return true;
    } else if (door_parameter_41 < 0 && i == 4 && j == 1) {
        return true;
    } else if (door_parameter_86 < 0 && i == 8 && j == 6) {
        return true;
    } else {
        // Ignore
    }

    /* Default: doors haven't moved and are all rendered */
    return false;
}

static bool check_height(float min_height, float max_height) 
{
    /* Player height validation */
    if (camera_pos[1] >= min_height && camera_pos[1] <= max_height) {
        return true;
    } else {
        return false;
    }
}

static void check_player_position()
{
    int i, j, tmp;
    float min_height, max_height;

    tmp = map_rows + camera_pos[2] / CUBE_SIZE;
    /* Map matrix positions */
    i = tmp > 10 ? 10 : tmp; // Row 10 bug fix
    j = camera_pos[0] / CUBE_SIZE;

    min_height = (map[i][j].height - 1) * CUBE_SIZE;
    max_height = map[i][j].height * CUBE_SIZE + CUBE_SIZE;

    if (map[i][j].type == 'l' && check_height(min_height, max_height)) {
        fprintf(stdout, "You died!\n");
        exit(EXIT_SUCCESS);
    } else if (map[i][j].type == 'k' && check_height(min_height, max_height)) {
        if (i == 1 && j == 1) {
            has_key_11 = true;
        } else if (i == 2 && j == 3) {
            has_key_23 = true;
        } else if (i == 7 && j == 1) {
            has_key_71 = true;
        } else if (i == 9 && j == 9) {
            has_key_99 = true;
        } else {
            // Ignore
        }
    } else if (map[i][j].type == 's' && check_height(min_height, max_height)) {
        if (i == 4 && j == 4) {
            has_switch_44 = true;
        } else if (i == 7 && j == 3) {
            has_switch_73 = true;
        } else if (i == 7 && j == 7) {
            has_switch_77 = true;
        } else if ( i == 9 && j == 8) {
            has_switch_98 = true;
        } else {
            // Ignore
        }
    }

}

static bool check_inside_circle(int i, int j)
{
    /* Coordinates of the center of the cube */
    float x_center = j * CUBE_SIZE + CUBE_SIZE / 2;
    float z_center = -(map_rows - 1 - i) * CUBE_SIZE - CUBE_SIZE / 2;

    /* Player current position */
    float x_player = camera_pos[0];
    float z_player = camera_pos[2];

    /* Teleport inner radius, squared */
    float r_in_square = (0.75 * CUBE_SIZE / 2) * (0.75 * CUBE_SIZE / 2);

    /* Player distance, squared */
    float d_square = (x_player - x_center) * (x_player - x_center) 
                   + (z_player - z_center) * (z_player - z_center);

    return d_square <= r_in_square ? true : false;
}

static void check_teleportation()
{
    int i, j, tmp;
    float min_height, max_height;

    tmp = map_rows + camera_pos[2] / CUBE_SIZE;
    /* Map matrix positions */
    i = tmp > 10 ? 10 : tmp; // Row 10 bug fix
    j = camera_pos[0] / CUBE_SIZE;

    min_height = (map[i][j].height - 1) * CUBE_SIZE;
    max_height = map[i][j].height * CUBE_SIZE;

    /* Checking player position map type. If teleport, teleports player to proper position */
    if ( (map[i][j].type == 'g' || map[i][j].type == 'b' || map[i][j].type == 'p'
         || map[i][j].type == 'r' || map[i][j].type == 'm' || map[i][j].type == 'c'
         || map[i][j].type == 'y' || map[i][j].type == 'o') 
         && check_inside_circle(i, j) && check_height(min_height, max_height) )
    {
        int to_row = map[i][j].to_row;
        int to_col = map[i][j].to_col;

        /* Calculating next player position via map matrix data (to_row and to_col values) */
        float to_x = to_col * CUBE_SIZE + CUBE_SIZE / 2;
        float to_z = -(map_rows - 1 - to_row) * CUBE_SIZE - CUBE_SIZE / 2;
        float to_height = (map[to_row][to_col].height - 1) * CUBE_SIZE + CUBE_SIZE / 2;

        /* Updating player position vector */
        glm_vec3((vec3){to_x, to_height, to_z}, camera_pos);
    }
}

static void create_map()
{
    int i, j;

    float elevator_scale_factor = 0.15;

    /* Special factor that fixes the elevator position since scaling
     * will cause the elevator to float in space */
    float e_scale_move_factor = 0.8 * elevator_scale_factor * CUBE_SIZE * CUBE_SIZE;

    glPushMatrix();

        glTranslatef(CUBE_SIZE / 2, - CUBE_SIZE / 2, - CUBE_SIZE / 2);

        for (i = map_rows - 1; i >= 0; i--) {
            for (j = 0; j < map_cols; j++) {

                /* x and z coordinates of the CENTER of the cube */
                float x = j*CUBE_SIZE;
                float z = -(map_rows - 1 - i) * CUBE_SIZE;

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
                        if (!check_door_moved(i, j)) {
                            glPushMatrix();
                                glTranslatef(x, map[i][j].height * CUBE_SIZE, z);

                                move_door(i, j);

                                set_diffuse(0.5, 0.2, 0.1, 1);
                                glutSolidCube(CUBE_SIZE);
                            glPopMatrix();
                        }
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
                            glTranslatef(x, map[i][j].height * CUBE_SIZE, z);
                            glTranslatef(0, -e_scale_move_factor, 0);

                            move_elevator(i, j, elevator_scale_factor);

                            glScalef(1, elevator_scale_factor, 1);
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
                        if (check_key_inventory(i, j)) {
                            glPushMatrix();
                                glTranslatef(x, map[i][j].height * CUBE_SIZE, z);
                                set_diffuse(0.8, 0.8, 0, 1);

                                glTranslatef(0, CUBE_SIZE / 5 * sin(2 * global_time_parameter * DEG_TO_RAD), 0);
                                glRotatef(-global_time_parameter * 2, 0, 1, 0);

                                create_key();
                            glPopMatrix();
                        }
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
                        if (check_switch_inventory(i, j)) {
                            glPushMatrix();
                                glTranslatef(x, map[i][j].height * CUBE_SIZE, z);
                                glTranslatef(0, - CUBE_SIZE / 2.5, 0);

                                /* Rotating switch around y-axis */
                                glRotatef(global_time_parameter * 2, 0, 1, 0);

                                glRotatef(-25, 0, 0, 1);
                                set_diffuse(0.5, 0.5, 0.7, 1);
                                create_switch();
                            glPopMatrix();
                        }
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

static void on_display(void)
{
    /* GLfloat light_position[] = {eye_x, eye_y, eye_z, 1}; */

    /* Clearing the previous window appearance */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* Calculating right vector and directional vector */
    glm_vec3_crossn(camera_front, camera_up, camera_right);
    glm_vec3_add(camera_pos, camera_front, camera_direction);

    /* Cammera settings */
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    gluLookAt(camera_pos[0], camera_pos[1], camera_pos[2],
              camera_direction[0], camera_direction[1], camera_direction[2],
              camera_up[0], camera_up[1], camera_up[2]);

    draw_axis();

    create_map();

    glutSwapBuffers();
}