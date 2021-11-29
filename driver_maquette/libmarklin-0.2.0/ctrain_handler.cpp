/*
 * Fichier          : ctrain_handler.cpp
 * Auteur           : Magali Fröhlich (MFH)
 *
 * Date de creation : 11.4.2016
 *
 * But              : Implémentation de librairie qui gère la maquette de
 *                    trains.
 *
 *                    Ces fonctions permettent d'envoyer des commandes Maklin
 *                    à la maquettes :
 *
 *                    void mettre_fonction_loco(int no_loco, char etat);
 *                    - Cette fonction implémente la commande f1 qui allume
 *                      les phares.
 *
 *                    void mettre_vitesse_loco(int no_loco, int vitesse);
 *                    void mettre_vitesse_progressive(int no_loco,
 *                                                    int vitesse_future);
 *                    - Ces deux fonctions ont pour l'instant le même
 *                      comportement. Il semblerait qu'il soit possible dans
 *                      certains cas de désactiver la vitesse progressive mais
 *                      ce n'est pas implémenté ici.
 *
 *                    void arreter_loco(int no_loco);
 *                    - Cette fonction met la vitesse de la loco à 0.
 *
 *                    void inverser_sens_loco(int no_loco);
 *                    - La loco est d'abord arrêtée. Le sens est inversé puis la 
 *                      loco redémarre avec sa vitesse initale. 
 *                      (La vitesse de la loco est sauvée à chaque appel de fonction)
 *                    
 *
 *                    void diriger_aiguillage(int no_aiguillage,
 *                                            int direction,
 *                                            int temps_alim);
 *                    - Cette fonction permet d'activer les bobines qui bougent
 *                      les aiguillages. Le paramètre temps_alim indique le
 *                      temps qu'il faut à la bobine pour bouger l'aiguillage.
 *
 *                    Ces fonctions permettent de lire les contacts :
 *                    void* lire_contacts(void *arg);
 *                    void attendre_contact(int no_contact);
 *                    - La première fonction lit périodiquement la valeur des
 *                      contacts grâce à la commande bas niveau read_contact.
 *                      Cette commande permet de lire l'états de tous les
 *                      contacts.
 *
 *                    Ces fonctions gèrent l'initialisation / la fin du
 *                    programme :
 *                    void init_maquette(void);
 *                    - Cette fonction doit être appelée au début du programme
 *                      client. Elle s'occupe d'initialiser la communication
 *                      USB avec la maquette. Cette fonction se termine par
 *                      l'exécution de la commande "GO" qui donne le feu
 *                      vert aux les locos.
 *
 *                    void mettre_maquette_en_service(void);
 *                    - identique à init_maquette();
 *
 *                    void mettre_maquette_hors_service(void);
 *                    - Cette fonction effectue un arrêt d'urgence et termine
 *                      la communication USB. Elle doit être appelée par à la
 *                      fin du programme client.
 *
 *                    void demander_loco(int contact_a,
 *                                       int contact_b,
 *                                       int *no_loco,
 *                                       int *vitesse);
 *                     - Cette fonction ne fait rien. Il n'y aucun moyen
 *                       physique de récupérer l'adresse des locos.
 *                       En revanche,les locos peuvent être configurées par
 *                       une console Marklin. L'adresse est donc configurée
 *                       en dure et c'est la responsabilité de l'utilisateur
 *                       de placer les locosmotives correctement sur la
 *                       maquette.
 *
 * Revision         :
 *
 */

#include "ctrain_handler.h"
#include "redsusb.h"

#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "pthread.h"
#include "unistd.h"

#define MAQTRAIN_VENDOR_ID 0xee08
#define MAQTRAIN_PRODUCT_ID 0x0540

#define MAQTRAIN_PREMIERE_ADRESSE_AIGUILLAGES 1
#define MAQTRAIN_DERNIERE_ADRESSE_AIGUILLAGES 255

#define MAQTRAIN_PREMIERE_ADRESSE_LOCOS 1
#define MAQTRAIN_DERNIERE_ADRESSE_LOCOS 80
#define MAQTRAIN_NB_LOCOS 80

#define MAQTRAIN_VITESSE_MIN 1
#define MAQTRAIN_VITESSE_MAX 14

#define MAQTRAIN_RAFRAICHISSEMENT_CONTACTS 300

#define MAQTRAIN_PREMIERE_ADRESSE_CONTACTS 1
#define MAQTRAIN_DERNIERE_ADRESSE_CONTACTS MAQTRAIN_NB_SENSORS

#define MAQTRAIN_TEST_AIGUILLAGES_INPUT(no_aiguillage) \
if(no_aiguillage < MAQTRAIN_PREMIERE_ADRESSE_AIGUILLAGES || \
   no_aiguillage > MAQTRAIN_DERNIERE_ADRESSE_AIGUILLAGES) \
        return;

#define MAQTRAIN_TEST_CONTACTS_INPUT(no_contact) \
if(no_contact < MAQTRAIN_PREMIERE_ADRESSE_CONTACTS || \
   no_contact > MAQTRAIN_DERNIERE_ADRESSE_CONTACTS) \
        return;

#define MAQTRAIN_TEST_ADRESSES_LOCOS_INPUT(no_loco) \
if(no_loco < MAQTRAIN_PREMIERE_ADRESSE_LOCOS || \
   no_loco > MAQTRAIN_DERNIERE_ADRESSE_LOCOS) \
        return;

#define MAQTRAIN_TEST_VITESSES_LOCOS_INPUT(vitesse_future) \
if(vitesse_future < MAQTRAIN_VITESSE_MIN || \
   vitesse_future > MAQTRAIN_VITESSE_MAX) \
        return;

static int maquette_en_service = 0;

static uint8_t contacts[MAQTRAIN_NB_SENSORS];

static pthread_t lecteur_contact;
static pthread_mutex_t mutex_contact = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t condition_contact  = PTHREAD_COND_INITIALIZER;

static int vitesse_locos[MAQTRAIN_NB_LOCOS];

/*
 * Thread qui à la charge de rafraichir l'état des contacts (actif/inactif).
 * Les contacts sont activés lorsqu'un locomotive passe dessus.
 *
 * Le taux de rafraichissement est définie par : MAQTRAIN_RAFRAICHISSEMENT_CONTACTS.
 * en usec.
 *
 * Le Thread est lancé par la fonction init_maquette() et terminé par la fonction
 * mettre_maquette_hors_service().
 *
 * La fonction qui lit les contacts est attendre_contact(no_contact).
 *
 */
void* lire_contacts(void* /* arg */) {

    uint8_t cache[MAQTRAIN_NB_SENSORS];
    memset(cache, 0, sizeof(uint8_t) * MAQTRAIN_NB_SENSORS);

    while(maquette_en_service) {

        pthread_mutex_lock(&mutex_contact);

        memcpy (cache, contacts, sizeof(uint8_t)*MAQTRAIN_NB_SENSORS);

        if(maqtrain_read_sensors(contacts) < 0) {
            pthread_exit(NULL);
        }

        /* Une locomotive a activé un contact */
        if(memcmp (contacts, cache, sizeof(uint8_t)*MAQTRAIN_NB_SENSORS) != 0) {
            pthread_cond_broadcast(&condition_contact);
        }

        pthread_mutex_unlock(&mutex_contact);

        usleep(MAQTRAIN_RAFRAICHISSEMENT_CONTACTS);
    }

    // avoid warning on non-void return
    pthread_exit(NULL);
}

void init_maquette(void) {

    if(maquette_en_service == 0) {

        usb_set_device(MAQTRAIN_VENDOR_ID, MAQTRAIN_PRODUCT_ID);

        maquette_en_service = 1;

        if (pthread_mutex_init(&mutex_contact, NULL) != 0) {
            return;
        }

        memset(contacts, 0, sizeof(uint8_t) * MAQTRAIN_NB_SENSORS);
        memset(vitesse_locos, 0, sizeof(int) * MAQTRAIN_NB_LOCOS);

        /* Initialise le thread de lecture des contacts */
        if (pthread_create(&lecteur_contact, NULL, &lire_contacts, NULL) < 0) {
            return;
        }

        /* Donne le feu vert aux locomotives */
        uint8_t data = 0x60;
        maqtrain_send_command(0, data);
    }
}

void mettre_maquette_hors_service(void) {

    if(maquette_en_service == 1) {

        maquette_en_service = 0;

        /* Arrêt d'urgence */
        uint8_t data = 0x61;
        maqtrain_send_command(0, data);

        pthread_join(lecteur_contact, NULL);
        pthread_mutex_destroy(&mutex_contact);

        //ev. close MaqTrain connection if needed (not the case yet)
    }
}

void mettre_maquette_en_service(void) {

    if(maquette_en_service == 0) {
        init_maquette();
    }
}

void diriger_aiguillage(int no_aiguillage, int direction, int temps_alim) {
	
    MAQTRAIN_TEST_AIGUILLAGES_INPUT(no_aiguillage)

    if(direction != 0 && direction != 1)
        return;

    if(temps_alim < 0)
        return;

    /* Commande qui active la bobine qui bouge l'aiguillage */
    uint8_t data = 0x20;
    data |= (1 << !direction);

    maqtrain_send_command((uint8_t)no_aiguillage, data);

    usleep(temps_alim);

    /* Commande d'arrêt de la bobine qui bouge l'aiguillage */
    data = 0x20;
    maqtrain_send_command(0, data);
}

void attendre_contact(int no_contact) {

    MAQTRAIN_TEST_CONTACTS_INPUT(no_contact)

    pthread_mutex_lock(&mutex_contact);

    while(contacts[no_contact-1] != 1) {
        pthread_cond_wait(&condition_contact, &mutex_contact);
    }

    pthread_mutex_unlock(&mutex_contact);
}

void arreter_loco(int no_loco) {

    MAQTRAIN_TEST_ADRESSES_LOCOS_INPUT(no_loco)

    /* Commande qui met la vitesse de la loco à 0 */
    maqtrain_send_command((uint8_t)no_loco, 0);
}

void mettre_vitesse_progressive(int no_loco, int vitesse_future) {

    MAQTRAIN_TEST_ADRESSES_LOCOS_INPUT(no_loco)
    MAQTRAIN_TEST_VITESSES_LOCOS_INPUT(vitesse_future)

    vitesse_locos[no_loco-1] = vitesse_future;


// TODO : A tester.
//
//    /* Commande qui désactive la temporisation du freinage/accélération */
//    uint8_t data = 0x40;
//    maqtrain_send_command((uint8_t)no_loco, data);

    /* Commande qui donne une vitesse à la loco */
    maqtrain_send_command((uint8_t)no_loco, (uint8_t)vitesse_future);
}

void mettre_fonction_loco(int no_loco, char etat) {

    MAQTRAIN_TEST_ADRESSES_LOCOS_INPUT(no_loco)

    if(etat != 0 || etat != 1)
        return;

    /* Commande qui allume les phares de la loco */
    uint8_t commande = 0x40;

    uint8_t data = etat;
    data |= commande;

    maqtrain_send_command((uint8_t)no_loco, data);
}

void inverser_sens_loco(int no_loco) {

    MAQTRAIN_TEST_ADRESSES_LOCOS_INPUT(no_loco)

    /* Commande qui inverse le sens de la loco */
    uint8_t data = 15;
    maqtrain_send_command((uint8_t)no_loco, 0);
    maqtrain_send_command((uint8_t)no_loco, data);
    maqtrain_send_command((uint8_t)no_loco, vitesse_locos[no_loco-1]);
}

void mettre_vitesse_loco(int no_loco, int vitesse) {

    MAQTRAIN_TEST_ADRESSES_LOCOS_INPUT(no_loco)
    MAQTRAIN_TEST_VITESSES_LOCOS_INPUT(vitesse)
   
    vitesse_locos[no_loco-1] = vitesse;

    /* Commande qui donne une vitesse à la loco */
    maqtrain_send_command((uint8_t)no_loco, (uint8_t)vitesse);
}

void demander_loco(int /*contact_a*/,
                   int /*contact_b*/,
                   int* /*no_loco*/,
                   int* /*vitesse*/ ) { }
