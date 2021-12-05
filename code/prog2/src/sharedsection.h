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

        // Save request information.
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

        afficher_message(qPrintable(QString("The engine no. %1 with id %2 requested the shared section from entry %3.")
                                    .arg(loco.numero())
                                    .arg(locoId == LocoId::LA ? "A" : "B")
                                    .arg(entryPoint == EntryPoint::EA ? "A" : "B")
        ));
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
            // The locomotive hasn't access to the shared section
            // it must wait until the section is free.
            isWaiting = true;
            mutex.release();

            loco.afficherMessage("I can not access the section.");
            loco.arreter();
            blocking.acquire();
            // The mutex is passed from the leaving loco.
            loco.demarrer();
        } else {
            // The locomotive has access to the shared section,
            // mark the section as occupied.
            loco.afficherMessage("I can access the section.");
            occupied = true;
        }

        // Remove the request.
        switch(locoId) {
            case LocoId::LA:
                locoARequest = false;
                break;
            case LocoId::LB:
                locoBRequest = false;
                break;
        }

        mutex.release();

        afficher_message(qPrintable(QString("The engine no. %1 with id %2 accesses the shared section.").arg(loco.numero()).arg(locoId == LocoId::LA ? "A" : "B")));
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
            // Liberate the loco that is currently waiting.
            isWaiting = false;
            blocking.release();
            // The mutex is passed to the new accessing loco.
        } else {
            // There is no more train on the shared section.
            occupied = false;
            mutex.release();
        }

        afficher_message(qPrintable(QString("The engine no. %1 with id %2 leaves the shared section.").arg(loco.numero()).arg(locoId == LocoId::LA ? "A" : "B")));
    }

private:
    PcoSemaphore blocking, mutex;
    EntryPoint locoAEntry, locoBEntry;
    bool locoARequest, locoBRequest, occupied, isWaiting;


    /**
     * @brief canAccess Determine if the given locomotive can access
     * to the shared section.
     *
     * @param locoId The locomotive id that want to access.
     * @return True is the access is granted, false otherwise.
     */
    bool canAccess(LocoId locoId) {
        // If there is already a loco, the access is denied.
        if (occupied) {
            return false;
        }

        // If the two locomotive requested the access, the authorization
        // depends on which entry the two loco came in.
        if (locoARequest && locoBRequest) {
            if (locoId == LocoId::LA) {
                // Access is granted to loco A when the two loco come from the same entry.
                return locoAEntry == locoBEntry;
            } else {
                // Access is granted to loco B when the two loco come from different entry.
                return locoAEntry != locoBEntry;
            }
        }

        // The other loco hasn't requested access.
        return true;
    }
};


#endif // SHAREDSECTION_H
