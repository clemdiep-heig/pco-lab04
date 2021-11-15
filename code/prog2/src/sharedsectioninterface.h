//    ___  _________    ___  ___  ___   __ //
//   / _ \/ ___/ __ \  |_  |/ _ \|_  | / / //
//  / ___/ /__/ /_/ / / __// // / __/ / /  //
// /_/   \___/\____/ /____/\___/____//_/   //
//                                         //
// Rien à modifier ici

#ifndef SHAREDSECTIONINTERFACE_H
#define SHAREDSECTIONINTERFACE_H

#include "locomotive.h"

/**
 * @brief La classe SharedSectionInterface est une interface (classe abstraite pure) qui définit
 * trois méthodes, request, getAccess et leave afin de gérer l'accès à une section partagée d'un
 * parcours de locomotives.
 */
class SharedSectionInterface
{

public:

    /**
     * @brief An enum to represent the locomotive id
     */
    enum class LocoId {
        LA,
        LB
    };

    /**
     * @brief An enum to represent the entry point
     */
    enum class EntryPoint {
        EA,
        EB
    };

    /**
     * @brief request Méthode a appeler pour indiquer que la locomotive désire accéder à la
     * section partagée (deux contacts avant la section partagée).
     * @param loco La locomotive qui désire accéder
     * @param locoId L'identidiant de la locomotive qui fait l'appel
     * @param entryPoint Le point d'entree de la locomotive qui fait l'appel
     */
    virtual void request(Locomotive& loco, LocoId locoId, EntryPoint entryPoint) = 0;

    /**
     * @brief getAccess Méthode à appeler pour accéder à la section partagée, doit arrêter la
     * locomotive et mettre son thread en attente si la section est occupée ou va être occupée
     * par une locomotive de plus haute priorité. Si la locomotive et son thread ont été mis en
     * attente, le thread doit être reveillé lorsque la section partagée est à nouveau libre et
     * la locomotive redémarée. (méthode à appeler un contact avant la section partagée).
     * @param loco La locomotive qui essaie accéder à la section partagée
     * @param locoId L'identidiant de la locomotive qui fait l'appel
     */
    virtual void getAccess(Locomotive& loco, LocoId locoId) = 0;

    /**
     * @brief leave Méthode à appeler pour indiquer que la locomotive est sortie de la section
     * partagée. (reveille les threads des locomotives potentiellement en attente).
     * @param loco La locomotive qui quitte la section partagée
     * @param locoId L'identidiant de la locomotive qui fait l'appel
     */
    virtual void leave(Locomotive& loco, LocoId locoId) = 0;
};

#endif // SHAREDSECTIONINTERFACE_H
