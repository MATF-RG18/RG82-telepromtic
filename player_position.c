static void set_player_position(int i, int j)
{
    /* Center of the cube and proper height */
    float x = j * CUBE_SIZE + CUBE_SIZE / 2;
    float z = -(map_rows - 1 - i) * CUBE_SIZE - CUBE_SIZE / 2;
    float y = map[i][j].height * CUBE_SIZE;

    /* Setting player position via global vectors */
    glm_vec3((vec3){x, y, z}, camera_pos);
    glm_vec3((vec3){0, 0, z - 1}, camera_front);
}

case '@':
                        /* Grass */
                        glPushMatrix();
                            glTranslatef(x, 0, z);
                            set_diffuse(0.2, 0.7, 0.1, 1);
                            glutSolidCube(CUBE_SIZE);
                        glPopMatrix();

                        if (!starting_position) {
                            starting_position = true;
                            set_player_position(i, j);
                        }
                        break;