//    ___  _________    ___  ___  ___   __ //
//   / _ \/ ___/ __ \  |_  |/ _ \|_  | / / //
//  / ___/ /__/ /_/ / / __// // / __/ / /  //
// /_/   \___/\____/ /____/\___/____//_/   //
//                                         //


#ifndef SHAREDSECTION_H
#define SHAREDSECTION_H

#include <QDebug>

#include <pcosynchro/pcosemaphore.h>

#include "locomotive.h"
#include "ctrain_handler.h"
#include "sharedsectioninterface.h"

/**
 * @brief La classe SharedSection implémente l'interface SharedSectionInterface qui
 * propose les méthodes liées à la section partagée.
 */
class SharedSection final : public SharedSectionInterface
{
public:

    /**
     * @brief SharedSection Constructeur de la classe qui représente la section partagée.
     * Initialisez vos éventuels attributs ici, sémaphores etc.
     */
    SharedSection(): blocking(0), mutex(1), locoAEntry(EntryPoint::EA), locoBEntry(EntryPoint::EA),
        locoARequest(false), locoBRequest(false), occupied(false), isWaiting(false) {
    }

    /**
     * @brief request Méthode a appeler pour indiquer que la locomotive désire accéder à la
     * section partagée (deux contacts avant la section partagée).
     * @param loco La locomotive qui désire accéder
     * @param locoId L'identidiant de la locomotive qui fait l'appel
     * @param entryPoint Le point d'entree de la locomotive qui fait l'appel
     */
    void request(Locomotive& loco, LocoId locoId, EntryPoint entryPoint) override {
        mutex.acquire();

        switch(locoId) {
            case LocoId::LA:
                locoARequest = true;
                locoAEntry = entryPoint;
                break;
            case LocoId::LB:
                locoBRequest = true;
                locoBEntry = entryPoint;
                break;
        }

        mutex.release();

        // Exemple de message dans la console globale
        afficher_message(qPrintable(QString("The engine no. %1 requested the shared section.").arg(loco.numero())));
    }

    /**
     * @brief getAccess Méthode à appeler pour accéder à la section partagée, doit arrêter la
     * locomotive et mettre son thread en attente si la section est occupée ou va être occupée
     * par une locomotive de plus haute priorité. Si la locomotive et son thread ont été mis en
     * attente, le thread doit être reveillé lorsque la section partagée est à nouveau libre et
     * la locomotive redémarée. (méthode à appeler un contact avant la section partagée).
     * @param loco La locomotive qui essaie accéder à la section partagée
     * @param locoId L'identidiant de la locomotive qui fait l'appel
     */
    void getAccess(Locomotive &loco, LocoId locoId) override {

        mutex.acquire();

        if (!canAccess(locoId)) {
            isWaiting = true;
            mutex.release();

            loco.arreter();
            blocking.acquire();
            loco.demarrer();
        } else {
            switch(locoId) {
                case LocoId::LA:
                    locoARequest = false;
                    break;
                case LocoId::LB:
                    locoBRequest = false;
                    break;
            }

            occupied = true;

            mutex.release();
        }

        // Exemple de message dans la console globale
        afficher_message(qPrintable(QString("The engine no. %1 accesses the shared section.").arg(loco.numero())));
    }

    /**
     * @brief leave Méthode à appeler pour indiquer que la locomotive est sortie de la section
     * partagée. (reveille les threads des locomotives potentiellement en attente).
     * @param loco La locomotive qui quitte la section partagée
     * @param locoId L'identidiant de la locomotive qui fait l'appel
     */
    void leave(Locomotive& loco, LocoId locoId) override {
        mutex.acquire();

        if (isWaiting) {
            isWaiting = false;
            locoBRequest = locoARequest = false;
            blocking.release();
        } else {
            occupied = false;
        }
        mutex.release();

        // Exemple de message dans la console globale
        afficher_message(qPrintable(QString("The engine no. %1 leaves the shared section.").arg(loco.numero())));
    }

    /* A vous d'ajouter ce qu'il vous faut */

private:
    PcoSemaphore blocking, mutex;
    EntryPoint locoAEntry, locoBEntry;
    bool locoARequest, locoBRequest, occupied, isWaiting;


    bool canAccess(SharedSectionInterface::LocoId locoId) {
        if (occupied) {
            return false;
        }

        if (locoARequest && locoBRequest) {
            if (locoId == SharedSectionInterface::LocoId::LA) {
                if (locoAEntry != locoBEntry) {
                    return false;
                }
            } else {
                if (locoAEntry == locoBEntry) {
                    return false;
                }
            }
        }

        return true;
    }
};


#endif // SHAREDSECTION_H
