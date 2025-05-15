#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include <tlhelp32.h>

// Game constants
#define MAX_HEALTH 100
#define MAX_HUNGER 100
#define MAX_REPUTATION 100
#define MAX_DAYS 365
#define MAX_NPCS 20
#define EVENT_INTERVAL 30000
#define MAX_ITEMS 10
#define MAX_INVENTORY 5

// Item types
typedef enum {
    KEY,
    TOOL,
    WEAPON,
    MEDICINE,
    FOOD,
    Document
} ItemType;

// Room types
typedef enum {
    CELL,
    CORRIDOR,
    CAFETERIA,
    YARD,
    INFIRMARY,
    SHOWER,
    LIBRARY,
    WORKSHOP,
    OFFICE
} RoomType;

// NPC types
typedef enum {
    INMATE,
    GUARD,
    DOCTOR,
    WORKER
} NPCType;

// Event types
typedef enum {
    FIGHT,
    SEARCH,
    RIOT,
    MEDICAL_EMERGENCY,
    ESCAPE_ATTEMPT
} EventType;

// Add role type
typedef enum {
    PRISONER=0,
    GUARDIAN=1,
    MANAGER=2
} PlayerRole;

// Event structure
typedef struct {
    EventType type;
    char description[200];
    int severity;
    int duration;
    int is_active;
} GameEvent;

// Item structure
typedef struct {
    char name[50];
    ItemType type;
    int value;
    int is_hidden;
    int is_empty;  // Add this to track empty items
} Item;

// Room structure
typedef struct {
    RoomType type;
    char description[200];
    Item items[MAX_ITEMS];
    int item_count;
    int is_locked;
    int security_level;
} Room;

// NPC structure
typedef struct {
    char name[50];
    NPCType type;
    int health;
    int reputation;
    int relationship;
    int is_alive;
    Room *current_room;
    int is_busy;
} NPC;

// Add document structure
typedef struct {
    char name[50];
    char content[500];
    int is_secret;
} Document_Structure;

// Player structure
typedef struct {
    char name[50];
    int health;
    int hunger;
    int reputation;
    int money;
    int sentence;
    Item inventory[MAX_INVENTORY];
    int inventory_count;
    Room *current_room;
    int gang_affiliation;
    PlayerRole *role;
    Document_Structure documents[10];
    int document_count;
} Player;

// Game state structure
typedef struct {
    int current_day;
    char current_time[6];
    int hour;
    int minute;
    int is_game_over;
    NPC npcs[MAX_NPCS];
    int npc_count;
    GameEvent current_event;
    HANDLE event_thread;
    CRITICAL_SECTION game_lock;
    int is_paused;
    Room rooms[10];
    HANDLE npc_thread;
    int room_count;
} GameState;

// Function prototypes
void initialize_game(Player *player, GameState *state);
void initialize_rooms(GameState *state);
void initialize_items(GameState *state);
void initialize_npcs(GameState *state);
void print_status(Player *player,GameState *state);
void handle_command(Player *player, GameState *state);
void look_around(Player *player, GameState *state);
void check_door(Player *player, GameState *state);
void talk_guard(Player *player, GameState *state);
void search_bed(Player *player, GameState *state);
void show_inventory(Player *player);
void use_item(Player *player, GameState *state, char *item_name);
void craft_tool(Player *player, GameState *state);
void attack_guard(Player *player, GameState *state);
void hide(Player *player, GameState *state);
void move_room(Player *player, GameState *state, RoomType target_room);
void eat(Player *player);
void work(Player *player);
void fight(Player *player, GameState *state);
void help();
void clear_screen();
void list_rooms(GameState *state);
void goto_room(Player *player, GameState *state, char *room_name);
void talk_npc(Player *player, GameState *state, char *npc_name);
void show_current_location(Player *player);
void list_documents(Player *player);
void read_document(Player *player, char *doc_name);
void list_prisoners(GameState *state);
void lock_door(GameState *state, char *room_name);
void open_door(GameState *state, char *room_name);
void view_logs(GameState *state);
void set_alarm(GameState *state, char *status);
void write_document(Player *player, char *doc_name, char *content);
void exit_prison(Player *player, GameState *state);
#include <stdio.h>
#include <string.h>
#include <ctype.h>

int is_valid_key(const char *key) {
    char clean[17];
    int j = 0;
    for (int i = 0; key[i] != '\0'; i++) {
        if (key[i] == '-') continue;
        if (!isalnum(key[i])) return 0;
        clean[j++] = toupper(key[i]);
        if (j > 16) return 0;
    }
    clean[j] = '\0';

    if (strlen(clean) != 16) return 0;

    int ascii_sum = 0;
    int xor_sum = 0;
    for (int i = 0; i < 4; i++)
        ascii_sum += clean[i];
    for (int i = 4; i < 8; i++)
        xor_sum ^= clean[i];
    if (ascii_sum != xor_sum) return 0;

    int modsum = 0;
    for (int i = 4; i < 8; i++)
        modsum += clean[i];
    if ((modsum % 7) != 3) return 0;

    int digit_count = 0;
    for (int i = 12; i < 16; i++) {
        if (isdigit(clean[i])) digit_count++;
    }
    if (digit_count != 1) return 0;

    return 1;
}

void initialize_rooms(GameState *state) {
    state->room_count = 8;
    
    strcpy(state->rooms[0].description, "Your cell. A small, cramped space with a bed and a desk.");
    state->rooms[0].type = CELL;
    state->rooms[0].is_locked = 1;
    state->rooms[0].security_level = 3;
    
    strcpy(state->rooms[1].description, "A long corridor with security cameras and guards patrolling.");
    state->rooms[1].type = CORRIDOR;
    state->rooms[1].is_locked = 0;
    state->rooms[1].security_level = 5;
    
    strcpy(state->rooms[2].description, "The prison cafeteria. Other inmates are eating and talking.");
    state->rooms[2].type = CAFETERIA;
    state->rooms[2].is_locked = 0;
    state->rooms[2].security_level = 4;
    
    strcpy(state->rooms[3].description, "The prison yard. A place to socialize and exercise.");
    state->rooms[3].type = YARD;
    state->rooms[3].is_locked = 0;
    state->rooms[3].security_level = 4;
    
    strcpy(state->rooms[4].description, "The prison infirmary. Medical staff and equipment.");
    state->rooms[4].type = INFIRMARY;
    state->rooms[4].is_locked = 0;
    state->rooms[4].security_level = 6;
    
    strcpy(state->rooms[5].description, "The prison showers. A place to clean up.");
    state->rooms[5].type = SHOWER;
    state->rooms[5].is_locked = 0;
    state->rooms[5].security_level = 3;
    
    strcpy(state->rooms[6].description, "The prison library. Books and educational materials.");
    state->rooms[6].type = LIBRARY;
    state->rooms[6].is_locked = 0;
    state->rooms[6].security_level = 4;
    
    strcpy(state->rooms[7].description, "The prison workshop. Tools and materials for work.");
    state->rooms[7].type = WORKSHOP;
    state->rooms[7].is_locked = 0;
    state->rooms[7].security_level = 5;

    strcpy(state->rooms[8].description, "The manager office. A place assigned to manager");
    state->rooms[8].type = OFFICE;
    state->rooms[8].is_locked = 0;
    state->rooms[8].security_level = 5;


}

void initialize_items(GameState *state) {
    Item key = {"Cell Key", KEY, 1, 1};
    state->rooms[0].items[0] = key;
    state->rooms[0].item_count = 1;
    
    Item tool = {"Metal File", TOOL, 1, 1};
    state->rooms[7].items[0] = tool;
    state->rooms[7].item_count = 1;
    
    Item medicine = {"Painkillers", MEDICINE, 1, 0};
    state->rooms[4].items[0] = medicine;
    state->rooms[4].item_count = 1;
    
    Item food = {"Bread", FOOD, 1, 0};
    state->rooms[2].items[0] = food;
    state->rooms[2].item_count = 1;
}

void initialize_npcs(GameState *state) {
    state->npc_count = 8;
    
    strcpy(state->npcs[0].name, "Mike");
    state->npcs[0].type = INMATE;
    state->npcs[0].reputation = 80;
    state->npcs[0].relationship = 0;
    state->npcs[0].current_room = &state->rooms[0];

    
    strcpy(state->npcs[1].name, "Carlos");
    state->npcs[1].type = INMATE;
    state->npcs[1].reputation = 60;
    state->npcs[1].relationship = -10;
    state->npcs[1].current_room = &state->rooms[0];

    strcpy(state->npcs[2].name, "Officer Johnson");
    state->npcs[2].type = GUARD;
    state->npcs[2].reputation = 70;
    state->npcs[2].relationship = -20;
    state->npcs[2].current_room = &state->rooms[1];

    
    strcpy(state->npcs[3].name, "Sergeant Williams");
    state->npcs[3].type = GUARD;
    state->npcs[3].reputation = 90;
    state->npcs[3].relationship = -30;
    state->npcs[3].current_room = &state->rooms[1];

    
    strcpy(state->npcs[4].name, "Dr. Smith");
    state->npcs[4].type = DOCTOR;
    state->npcs[4].reputation = 80;
    state->npcs[4].relationship = 10;
    state->npcs[4].current_room = &state->rooms[4];

    
    strcpy(state->npcs[5].name, "Mr. Brown");
    state->npcs[5].type = WORKER;
    state->npcs[5].reputation = 60;
    state->npcs[5].relationship = 0;
    state->npcs[5].current_room = &state->rooms[0];

}

void help(Player *player) {
    printf("\nAvailable commands:\n");
    printf("help          - Show this help message\n");
    printf("lookaround    - Look around the current room\n");
    printf("checkdoor     - Check if the door is locked\n");
    printf("searchbed     - Search your bed for items\n");
    printf("inventory     - Show your inventory\n");
    printf("eat           - Eat some food\n");
    printf("work          - Do some work for money\n");
    printf("fight         - Fight another inmate\n");;
    printf("list_rooms    - List all rooms\n");
    printf("status        - Show status of player\n");
    printf("goto <room>   - move a room\n");
    printf("whereami      - Show your current location\n");
    printf("talk <npc>    - Talk to an NPC (no arg → guard)\n");
    printf("use <item>    - Use an item\n");
    printf("craft         - Craft a tool\n");

    
    if (*player->role == GUARDIAN || *player->role == MANAGER) {
        printf("\nGuardian commands:\n");
        printf("list_prisoners - Show list of prisoners\n");
        printf("lock_door <room> - Lock a specific room\n");
        printf("open_door <room> - Unlock a specific room\n");
        printf("view_logs     - Show prison activity logs\n");
        printf("set_alarm <on/off> - Activate/deactivate alarm\n");
    }
    
    if (*player->role == MANAGER) {
        printf("\nManager commands:\n");
        printf("write <doc> <text> - Write an official document\n");
        printf("exit_prison    - Exit the prison (requires manager clearance)\n");
    }
}

void list_rooms(GameState *state) {
    printf("\nAvailable rooms:\n");
    for (int i = 0; i < state->room_count; i++) {
        printf("- %s\n", state->rooms[i].description);
    }
}

void goto_room(Player *player, GameState *state, char *room_name) {
    for (int i = 0; i < state->room_count; i++) {
        if (strstr(state->rooms[i].description, room_name) != NULL) {
            if (!state->rooms[i].is_locked) {
                player->current_room = &state->rooms[i];
                printf("You move to %s\n", state->rooms[i].description);
                look_around(player, state);
            } else {
                printf("The door is locked.\n");
            }
            return;
        }
    }
    printf("Room not found.\n");
}

void talk_npc(Player *player, GameState *state, char *npc_name) {
    for (int i = 0; i < state->npc_count; i++) {
        if (strcmp(state->npcs[i].name, npc_name) == 0) {
            if (!state->npcs[i].is_alive) {
                printf("%s is not available.\n", npc_name);
                return;
            }
            if(player->current_room==state->npcs[i].current_room){

                printf("You talk to %s...\n", npc_name);
                switch(state->npcs[i].type) {
                    case INMATE:
                        printf("%s says: ", npc_name);
                        if (rand() % 2) {
                            printf("'Hey, got any cigarettes?'\n");
                        } else {
                            printf("'Keep your head down, new fish.'\n");
                        }
                        break;
                    case GUARD:
                        printf("%s says: ", npc_name);
                        if (rand() % 2) {
                            printf("'Back to your cell, inmate!'\n");
                        } else {
                            printf("'What do you want?'\n");
                        }
                        break;
                    case DOCTOR:
                        printf("%s says: ", npc_name);
                        if (rand() % 2) {
                            printf("'Are you feeling alright?'\n");
                        } else {
                            printf("'I'm busy right now.'\n");
                        }
                        break;
                    case WORKER:
                        printf("%s says: ", npc_name);
                        if (rand() % 2) {
                            printf("'Need something from the workshop?'\n");
                        } else {
                            printf("'I'm working here.'\n");
                        }
                        break;
                }
                return;
            }
            else{
                printf("you are alone in %s",player->current_room);
            }
        }
    }
    printf("NPC not found.\n");
}
void talk_guard(Player *player, GameState *state){
    printf("Mind your own business. You don't want to mess with us.\n");
}
void show_current_location(Player *player) {
    printf("\nCurrent location: %s\n", player->current_room->description);
}

void list_documents(Player *player) {
    printf("\nAvailable documents:\n");
    for (int i = 0; i < player->document_count; i++) {
        printf("- %s\n", player->documents[i].name);
    }
}

void read_document(Player *player, char *doc_name) {
    for (int i = 0; i < player->document_count; i++) {
        if (strcmp(player->documents[i].name, doc_name) == 0) {
            printf("\nReading %s...\n", doc_name);
            if (player->documents[i].is_secret) {
                printf("The document is secret and cannot be read.\n");
            } else {
                printf("The document contains: %s\n", player->documents[i].content);
            }
            return;
        }
    }
    printf("Document not found in your documents.\n");
}

void handle_command(Player *player, GameState *state) {
    char command[100];
    char *command_arr[100];
    int argc=0;

    printf("\nEnter command (type 'help' for available commands): ");
    fgets(command, 100, stdin);
    command[strcspn(command, "\n")] = 0;
    

    for (int i = 0; command[i]; i++) {
        command[i] = tolower(command[i]);
    }

    char *token = strtok(command, " ");
    int i=0;
    while(token!=NULL){
        command_arr[i]=token;
        token = strtok(NULL, " ");
        i++;
        argc++;
    }
    if (strcmp(command_arr[0], "help") == 0) {
        help(player);
    }
    else if (strcmp(command_arr[0], "lookaround") == 0) {
        look_around(player, state);
    }
    else if (strcmp(command_arr[0], "checkdoor") == 0) {
        check_door(player, state);
    }
    else if (strcmp(command_arr[0], "searchbed") == 0) {
        search_bed(player, state);
    }
    else if (strcmp(command_arr[0], "inventory") == 0) {
        show_inventory(player);
    }
    else if (strcmp(command_arr[0], "eat") == 0) {
        eat(player);
    }
    else if (strcmp(command_arr[0], "work") == 0) {
        work(player);
    }
    else if (strcmp(command_arr[0], "fight") == 0) {
        fight(player, state);
    }
    else if(strcmp(command_arr[0],"list_rooms")==0){
        list_rooms(state);
    }
    else if(strcmp(command_arr[0],"goto")==0){
        goto_room(player,state,command_arr[1]);
    }
    else if(strcmp(command_arr[0],"status")==0){
        print_status(player,state);
    }
    else if (strcmp(command_arr[0], "whereami")  == 0) {
        show_current_location(player);
    }
    else if (strcmp(command_arr[0], "talk") == 0) {
        if (argc>1) talk_npc(player, state, command_arr[1]);
        else talk_guard(player, state);
    }
    else if (strcmp(command_arr[0], "list_documents")==0) {
        list_documents(player);
    }
    else if (strcmp(command_arr[0], "read") == 0 && argc>1){
        read_document(player, command_arr[1]);
    }
    else if (strcmp(command_arr[0], "use") == 0 && argc>1){
        use_item(player, state,command_arr[1]);
    }
    else if (strcmp(command_arr[0], "list_prisoners") == 0) {
        if (*player->role == GUARDIAN || *player->role == MANAGER) {
            list_prisoners(state);
        } else {
            printf("You don't have permission to view prisoner list.\n");
        }
    }
    else if (strncmp(command_arr[0], "lock_door ", 10) == 0) {
        if (*player->role == GUARDIAN || *player->role == MANAGER) {
            lock_door(state, command_arr[0] + 10);
        } else {
            printf("You don't have permission to lock doors.\n");
        }
    }
    else if (strncmp(command_arr[0], "open_door ", 10) == 0) {
        if (*player->role == GUARDIAN || *player->role == MANAGER) {
            open_door(state, command_arr[0] + 10);
        } else {
            printf("You don't have permission to unlock doors.\n");
        }
    }
    else if (strcmp(command_arr[0], "view_logs") == 0) {
        if (*player->role == GUARDIAN || *player->role == MANAGER) {
            view_logs(state);
        } else {
            printf("You don't have permission to view logs.\n");
        }
    }
    else if (strncmp(command_arr[0], "set_alarm ", 10) == 0) {
        if (*player->role == GUARDIAN || *player->role == MANAGER) {
            set_alarm(state, command_arr[0] + 10);
        } else {
            printf("You don't have permission to control the alarm.\n");
        }
    }
    else if (strncmp(command_arr[0], "write ", 6) == 0) {
        if (*player->role == MANAGER) {
            char *doc_name = strtok(command_arr[0] + 6, " ");
            char *content = strtok(NULL, "");
            if (doc_name && content) {
                write_document(player, doc_name, content);
            } else {
                printf("Invalid format. Use: write <document_name> <text>\n");
            }
        } else {
            printf("You don't have permission to write documents.\n");
        }
    }
    else if (strcmp(command_arr[0], "exit_prison") == 0) {
        if (*player->role == MANAGER) {
            exit_prison(player, state);
        } else {
            printf("You don't have permission to exit the prison.\n");
        }
    }
    else {
        printf("Unknown command. Type 'help' for available commands.\n");
    }
}

void eat(Player *player) {
    if (player->hunger < MAX_HUNGER) {
        printf("You eat some food. Your hunger decreases.\n");
        player->hunger = MAX_HUNGER;
    } else {
        printf("You're not hungry.\n");
    }
}

void work(Player *player) {
    printf("You do some work and earn money.\n");
    player->money += 5;
    player->hunger -= 10;
    if (player->hunger < 0) player->hunger = 0;
}

void fight(Player *player, GameState *state) {
    printf("You look for someone to fight...\n");
    for (int i = 0; i < state->npc_count; i++) {
        if (state->npcs[i].type == INMATE && !state->npcs[i].is_busy) {
            printf("You challenge %s to a fight!\n", state->npcs[i].name);
            if (rand() % 100 < 50) {
                printf("You win the fight! Your reputation increases.\n");
                player->reputation += 5;
                state->npcs[i].relationship -= 20;
            } else {
                printf("You lose the fight! Your health decreases.\n");
                player->health -= 20;
                player->reputation -= 5;
            }
            return;
        }
    }
    printf("No one wants to fight right now.\n");
}

void print_status(Player *player,GameState *state) {
    printf("\n=== Prison Status ===\n");
    printf("Day: %d\n", state->current_day);
    printf("Health: %d/100\n", player->health);
    printf("Hunger: %d/100\n", player->hunger);
    printf("Reputation: %d/100\n", player->reputation);
    printf("Money: $%d\n", player->money);
    printf("time: %s", state->current_time);
    
    printf("\nInventory:\n");
    if (player->inventory_count == 0) {
        printf("(Empty)\n");
    } else {
        for (int i = 0; i < MAX_INVENTORY; i++) {
            if (!player->inventory[i].is_empty) {
                printf("- %s\n", player->inventory[i].name);
            }
        }
    }
    printf("===================\n\n");
}

void look_around(Player *player, GameState *state) {
    printf("\n%s\n", player->current_room->description);
    printf("Items in the room:\n");
    for (int i = 0; i < player->current_room->item_count; i++) {
        if (!player->current_room->items[i].is_hidden) {
            printf("- %s\n", player->current_room->items[i].name);
        }
    }
    printf("people in the room:\n");
    for(int i=0;i<state->npc_count;i++){
        if(state->npcs[i].current_room==player->current_room){
            printf("- %s\n",state->npcs[i].name);
        }
    }
}

void check_door(Player *player, GameState *state) {
    if (player->current_room->is_locked) {
        printf("The door is locked. Security level: %d\n", player->current_room->security_level);
    } else {
        printf("The door is unlocked.\n");
    }
}

void search_bed(Player *player, GameState *state) {
    if (player->current_room->type == CELL) {
        printf("You search your bed...\n");
        if (rand() % 100 < 30) {
            printf("You found a hidden item!\n");
            // Add random item to inventory
            if (player->inventory_count < MAX_INVENTORY) {
                Item new_item = {"Hidden Note", Document, 1, 0};
                player->inventory[player->inventory_count++] = new_item;
            }
        } else {
            printf("You found nothing.\n");
        }
    } else {
        printf("There's no bed to search here.\n");
    }
}

void show_inventory(Player *player) {
    printf("\nYour inventory:\n");
    if (player->inventory_count == 0) {
        printf("(Empty)\n");
    } else {
        for (int i = 0; i < MAX_INVENTORY; i++) {
            if (!player->inventory[i].is_empty) {
                printf("- %s\n", player->inventory[i].name);
            }
        }
    }
}

void use_item(Player *player, GameState *state, char *item_name) {
    for (int i = 0; i < player->inventory_count; i++) {
        if (strcmp(player->inventory[i].name, item_name) == 0) {
            if (player->inventory[i].type == KEY && player->current_room->is_locked) {
                printf("You use the key to unlock the door.\n");
                player->current_room->is_locked = 0;
                return;
            }
            printf("You can't use that item here.\n");
            return;
        }
    }
    printf(item_name);//    
    printf("You don't have that item.\n");
}

void craft_tool(Player *player, GameState *state) {
    printf("crafting");
}

void attack_guard(Player *player, GameState *state) {
    printf("You attempt to attack a guard...\n");
    if (rand() % 100 < 30) {
        printf("Success! The guard is temporarily disabled.\n");
        player->reputation += 10;
    } else {
        printf("The guard overpowered you!\n");
        player->health -= 30;
        player->reputation -= 20;
    }
}

void hide(Player *player, GameState *state) {
    printf("You attempt to hide...\n");
    if (rand() % 100 < 50) {
        printf("You successfully hide from view.\n");
        player->current_room->security_level -= 2;
    } else {
        printf("You failed to hide properly.\n");
    }
}

void move_room(Player *player, GameState *state, RoomType target_room) {
    for (int i = 0; i < state->room_count; i++) {
        if (state->rooms[i].type == target_room) {
            if (!state->rooms[i].is_locked) {
                player->current_room = &state->rooms[i];
                printf("You move to the new location.\n");
                look_around(player, state);
            } else {
                printf("The door is locked.\n");
            }
            return;
        }
    }
    printf("You can't move there.\n");
}

DWORD WINAPI event_thread_function(LPVOID lpParam) {
    GameState *state = (GameState *)lpParam;
    
    while (!state->is_game_over) {
        if (!state->is_paused) {
            EnterCriticalSection(&state->game_lock);
            
            if (rand() % 100 < 10) {
                state->current_event.type = rand() % 5;
                state->current_event.is_active = 1;
                state->current_event.severity = rand() % 10 + 1;
                state->current_event.duration = rand() % 60 + 30;
                
                switch (state->current_event.type) {
                    case FIGHT:
                        strcpy(state->current_event.description, "A fight has broken out in the yard!");
                        break;
                    case SEARCH:
                        strcpy(state->current_event.description, "Guards are conducting a cell search!");
                        break;
                    case RIOT:
                        strcpy(state->current_event.description, "A riot has started in the cafeteria!");
                        break;
                    case MEDICAL_EMERGENCY:
                        strcpy(state->current_event.description, "Medical emergency in the infirmary!");
                        break;
                    case ESCAPE_ATTEMPT:
                        strcpy(state->current_event.description, "An escape attempt has been detected!");
                        break;
                }
                
                printf("\n=== PRISON EVENT ===\n");
                printf("%s\n", state->current_event.description);
                printf("Severity: %d/10\n", state->current_event.severity);
                printf("Duration: %d seconds\n", state->current_event.duration);
                printf("===================\n");
            }
            
            LeaveCriticalSection(&state->game_lock);
        }
        
        Sleep(EVENT_INTERVAL);
    }
    
    return 0;
}

DWORD WINAPI npc_main_thread(LPVOID lpParam) {
    GameState *state = (GameState *)lpParam;

    while (!state->is_game_over) {
        for (int i = 0; i < state->npc_count; i++) {
            NPC *npc = &state->npcs[i];
            if (!npc->is_alive || npc->is_busy) continue;

            int rand_room = rand() % state->room_count;
            Room *target_room = &state->rooms[rand_room];

            if (npc->type == INMATE && target_room->type == OFFICE) continue;
            if ((npc->current_room && npc->current_room->is_locked) || target_room->is_locked) continue;

            npc->current_room = target_room;
            
            if (rand() % 100 < 20) {
                npc->is_busy = 1;

                switch (npc->type) {
                    case INMATE:
                        printf("%s is %s.\n", npc->name,
                            rand() % 2 ? "working in the workshop" : "reading in the library");
                        break;
                    case GUARD:
                        printf("%s is %s.\n", npc->name,
                            rand() % 2 ? "patrolling the cell block" : "checking the security cameras");
                        break;
                    case DOCTOR:
                        printf("%s is %s.\n", npc->name,
                            rand() % 2 ? "treating patients" : "updating medical records");
                        break;
                    case WORKER:
                        printf("%s is %s.\n", npc->name,
                            rand() % 2 ? "working in the kitchen" : "cleaning common areas");
                        break;
                }

                Sleep(rand() % 5000 + 2000);
                npc->is_busy = 0;
            }
        }

        Sleep((rand() % 10 + 5) * 1000);  
    }

    return 0;
}

DWORD WINAPI TimeThread(LPVOID lpParam){
    GameState *state = (GameState *)lpParam;
    

    while (state->current_day < 365) {
        if(state->hour<24){
            Sleep(1000); 

            state->minute++;    
            if (state->minute == 60) {
                state->minute = 0;
                state->hour++;
            }
            sprintf(state->current_time,"%02d:%02d\n", state->hour, state->minute);
        }
        else{
            state->hour=0;
            state->current_day++;

        }

    }
    return 0;


}
DWORD WINAPI HungerThread(LPVOID lpParam){
    Player *player = (Player *)lpParam;
    while(player->health > 0){
        Sleep(1*60*1000);
        player->hunger-=10;
    }
    
}
void cleanup_game(GameState *state) {
    state->is_game_over = 1;
    
    WaitForSingleObject(state->event_thread, INFINITE);

    WaitForSingleObject(state->npc_thread, INFINITE);
    
    DeleteCriticalSection(&state->game_lock);
    CloseHandle(state->event_thread);
    CloseHandle(state->npc_thread);
}

void list_prisoners(GameState *state) {
    printf("\nList of prisoners:\n");
    for (int i = 0; i < state->npc_count; i++) {
        if (state->npcs[i].type == INMATE && state->npcs[i].is_alive) {
            printf("- %s (Health: %d, Reputation: %d)\n", 
                   state->npcs[i].name, 
                   state->npcs[i].health,
                   state->npcs[i].reputation);
        }
    }
}

void lock_door(GameState *state, char *room_name) {
    for (int i = 0; i < state->room_count; i++) {
        if (strstr(state->rooms[i].description, room_name) != NULL) {
            state->rooms[i].is_locked = 1;
            printf("Door to %s is now locked.\n", state->rooms[i].description);
            return;
        }
    }
    printf("Room not found.\n");
}

void open_door(GameState *state, char *room_name) {
    for (int i = 0; i < state->room_count; i++) {
        if (strstr(state->rooms[i].description, room_name) != NULL) {
            state->rooms[i].is_locked = 0;
            printf("Door to %s is now unlocked.\n", state->rooms[i].description);
            return;
        }
    }
    printf("Room not found.\n");
}

void view_logs(GameState *state) {
    printf("\n=== Prison Activity Log ===\n");
    printf("Recent events:\n");
    if (state->current_event.is_active) {
        printf("- %s (Severity: %d/10)\n", 
               state->current_event.description,
               state->current_event.severity);
    }
    printf("\nCurrent status:\n");
    for (int i = 0; i < state->npc_count; i++) {
        if (state->npcs[i].type == INMATE) {
            printf("- %s: %s\n", 
                   state->npcs[i].name,
                   state->npcs[i].is_busy ? "Active" : "In cell");
        }
    }
    printf("==========================\n");
}

void set_alarm(GameState *state, char *status) {
    if (strcmp(status, "on") == 0) {
        printf("Alarm activated! All prisoners must return to their cells!\n");
        // Force all inmates to return to their cells
        for (int i = 0; i < state->npc_count; i++) {
            if (state->npcs[i].type == INMATE) {
                state->npcs[i].is_busy = 0;
            }
        }
    } else if (strcmp(status, "off") == 0) {
        printf("Alarm deactivated. Normal operations resumed.\n");
    } else {
        printf("Invalid alarm status. Use 'on' or 'off'.\n");
    }
}

void write_document(Player *player, char *doc_name, char *content) {
    if (player->document_count < 10) {
        strcpy(player->documents[player->document_count].name, doc_name);
        strcpy(player->documents[player->document_count].content, content);
        player->document_count++;
        printf("Document '%s' written successfully.\n", doc_name);
    } else {
        printf("You've reached the maximum number of documents.\n");
    }
}

void exit_prison(Player *player, GameState *state) {
    if (*player->role == MANAGER) {
        char password[50];
        printf("\n=== Prison Exit Protocol ===\n");
        printf("enter password:");
        fgets(password,50,stdin);

        if(is_valid_key(password)==1){
            state->is_game_over = 1;
            printf("You have successfully exited the prison.\n");
        }

       else  printf("wrong password.\n");
    } else {
        printf("You don't have permission to exit the prison.\n");
    }
}

void initialize_game(Player *player, GameState *state) {
    fflush(stdin);
    
    printf("Welcome to Prison Simulator!\n");
    printf("Enter your name: ");
    fgets(player->name, 50, stdin);
    player->name[strcspn(player->name, "\n")] = 0;
    
    // Set default role to prisoner
    player->role = malloc(sizeof(PlayerRole));

    *player->role = PRISONER;
    
    // Initialize player stats
    player->health = MAX_HEALTH;
    player->hunger = MAX_HUNGER;
    player->reputation = 50;
    player->money = 0;
    player->sentence = 365;
    player->gang_affiliation = 0;
    player->inventory_count = 0;
    player->document_count = 0;
    
    for (int i = 0; i < MAX_INVENTORY; i++) {
        strcpy(player->inventory[i].name, "");
        player->inventory[i].type = FOOD;
        player->inventory[i].value = 0;
        player->inventory[i].is_hidden = 0;
        player->inventory[i].is_empty = 1;
    }
    
    state->current_day = 1;
    state->hour=8;
    state->minute=0;
    sprintf(state->current_time,"%02d:%02d",state->hour,state->minute);
    state->is_game_over = 0;
    state->is_paused = 0;
    state->npc_count = 0;
    
    InitializeCriticalSection(&state->game_lock);
    
    initialize_rooms(state);
    initialize_items(state);
    initialize_npcs(state);
    
    player->current_room = &state->rooms[0]; 
    
    srand((unsigned int)time(NULL));
}
void func_a_deb_154345() {
    PROCESSENTRY32 entry = {0};
    entry.dwSize = sizeof(entry);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (Process32First(snapshot, &entry)) {
        do {
            if (strstr(entry.szExeFile, "x64dbg") || strstr(entry.szExeFile, "ollydbg") || strstr(entry.szExeFile, "windbg") || strstr(entry.szExeFile, "ghidra")) {
                MessageBoxA(NULL, "Debugger process found!", "Stop", MB_OK);
                ExitProcess(1);
            }
        } while (Process32Next(snapshot, &entry));
    }
}
// Add Windows entry point
#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpcommand_arrLine, int ncommand_arrShow) {
#else
int main() {
#endif
    func_a_deb_154345();
    Player player;
    GameState state;
    
    initialize_game(&player, &state);
    
    state.event_thread = CreateThread(NULL, 0, event_thread_function, &state, 0, NULL);
    CreateThread(NULL, 0, TimeThread, &state, 0, NULL);
    CreateThread(NULL, 0, HungerThread, &player, 0, NULL);

    

    state.npc_thread = CreateThread(NULL, 0, npc_main_thread, &state, 0, NULL);
    
    while (!state.is_game_over) {
        handle_command(&player, &state);
        
        if (player.hunger < 0) player.hunger = 0;
        
        if (player.health <= 0) {
            printf("\nGame Over! You died.\n");
            state.is_game_over = 1;
        }
        else if (state.current_day >= player.sentence) {
            printf("\nCongratulations! You have served your sentence.\n");
            state.is_game_over = 1;
        }
        
        Sleep(100);
    }
    
    cleanup_game(&state);
    
    return 0;
} 