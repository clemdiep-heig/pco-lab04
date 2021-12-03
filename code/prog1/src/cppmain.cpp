//    ___  _________    ___  ___  ___   __ //
//   / _ \/ ___/ __ \  |_  |/ _ \|_  | / / //
//  / ___/ /__/ /_/ / / __// // / __/ / /  //
// /_/   \___/\____/ /____/\___/____//_/   //
//

#include "ctrain_handler.h"

#include "locomotive.h"
#include "locomotivebehavior.h"
#include "sharedsectioninterface.h"
#include "sharedsection.h"

// Locomotives :
// Vous pouvez changer les vitesses initiales, ou utiliser la fonction loco.fixerVitesse(vitesse);
// Laissez les numéros des locos à 0 et 1 pour ce laboratoire

// Locomotive A
static Locomotive locoA(6 /* Numéro (pour commande trains sur maquette réelle) */, 10 /* Vitesse */);
// Locomotive B
static Locomotive locoB(10 /* Numéro (pour commande trains sur maquette réelle) */, 5 /* Vitesse */);

//Arret d'urgence
void emergency_stop()
{
    locoA.arreter();
    locoA.eteindrePhares();
    locoB.arreter();
    locoB.eteindrePhares();

    afficher_message("\nSTOP!");
}


//Fonction principale
int cmain()
{
    /************
     * Maquette *
     ************/
    //Choix de la maquette (A ou B)
    selection_maquette(MAQUETTE_A /*MAQUETTE_B*/);

    /**********************************
     * Initialisation des aiguillages *
     **********************************/
    diriger_aiguillage(1,  TOUT_DROIT, 0);
    diriger_aiguillage(2,  DEVIE     , 0);
    diriger_aiguillage(3,  DEVIE     , 0);
    diriger_aiguillage(4,  TOUT_DROIT, 0);
    diriger_aiguillage(5,  TOUT_DROIT, 0);
    diriger_aiguillage(6,  TOUT_DROIT, 0);
    diriger_aiguillage(7,  TOUT_DROIT, 0);
    diriger_aiguillage(8,  DEVIE     , 0);
    diriger_aiguillage(9,  TOUT_DROIT, 0);
    diriger_aiguillage(10, TOUT_DROIT, 0);
    diriger_aiguillage(11, TOUT_DROIT, 0);
    diriger_aiguillage(12, TOUT_DROIT, 0);
    diriger_aiguillage(13, TOUT_DROIT, 0);
    diriger_aiguillage(14, DEVIE     , 0);
    diriger_aiguillage(15, DEVIE     , 0);
    diriger_aiguillage(16, TOUT_DROIT, 0);
    diriger_aiguillage(17, TOUT_DROIT, 0);
    diriger_aiguillage(18, TOUT_DROIT, 0);
    diriger_aiguillage(19, TOUT_DROIT, 0);
    diriger_aiguillage(20, DEVIE     , 0);
    diriger_aiguillage(21, DEVIE     , 0);
    diriger_aiguillage(22, TOUT_DROIT, 0);
    diriger_aiguillage(23, DEVIE     , 0);
    diriger_aiguillage(24, TOUT_DROIT, 0);

    /********************************
     * Position de départ des locos *
     ********************************/

    // Loco 0
    locoA.fixerPosition(9, 35);

    // Loco 1
    locoB.fixerPosition(31, 1);

    /*************************************
     * Définitino des parcours des locos *
     *************************************/
    // Loco 0
    std::vector<Section> travelA;
    travelA.push_back({9, {
        std::make_pair<int, int>(5, DEVIE),
    }, false});

    travelA.push_back({34, {
        std::make_pair<int, int>(20, TOUT_DROIT),
        std::make_pair<int, int>(19, DEVIE),
        std::make_pair<int, int>(10, DEVIE),
    }, true});

    travelA.push_back({14, {
        std::make_pair<int, int>(5, TOUT_DROIT),
        std::make_pair<int, int>(20, DEVIE),
    }, false});

    travelA.push_back({35, {}, false});

    // Loco 1
    std::vector<Section> travelB;
    travelB.push_back({31, {}, false});

    travelB.push_back({30, {
        std::make_pair<int, int>(19, TOUT_DROIT),
        std::make_pair<int, int>(10, TOUT_DROIT),
    }, true}),

    travelB.push_back({11, {}, false});
    travelB.push_back({1, {}, false});

    /***********
     * Message *
     **********/

    // Affiche un message dans la console de l'application graphique
    afficher_message("Hit play to start the simulation...");

    /*********************
     * Threads des locos *
     ********************/

    // Création de la section partagée
    std::shared_ptr<SharedSectionInterface> sharedSection = std::make_shared<SharedSection>();

    // Création du thread pour la loco 0
    std::unique_ptr<Launchable> locoBehaveA = std::make_unique<LocomotiveBehavior>(locoA, sharedSection, travelA);
    // Création du thread pour la loco 1
    std::unique_ptr<Launchable> locoBehaveB = std::make_unique<LocomotiveBehavior>(locoB, sharedSection, travelB);

    // Lanchement des threads
    afficher_message(qPrintable(QString("Lancement thread loco A (numéro %1)").arg(locoA.numero())));
    locoBehaveA->startThread();
    afficher_message(qPrintable(QString("Lancement thread loco B (numéro %1)").arg(locoB.numero())));
    locoBehaveB->startThread();

    // Attente sur la fin des threads
    locoBehaveA->join();
    locoBehaveB->join();

    //Fin de la simulation
    mettre_maquette_hors_service();

    return EXIT_SUCCESS;
}
