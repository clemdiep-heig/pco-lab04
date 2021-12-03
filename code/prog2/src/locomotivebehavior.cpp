//    ___  _________    ___  ___  ___   __ //
//   / _ \/ ___/ __ \  |_  |/ _ \|_  | / / //
//  / ___/ /__/ /_/ / / __// // / __/ / /  //
// /_/   \___/\____/ /____/\___/____//_/   //
//                                         //


#include "locomotivebehavior.h"
#include "ctrain_handler.h"

const int NB_TURN = 2;

template<typename iterator>
void LocomotiveBehavior::doTravel(iterator begin, iterator end, bool isReverse) {
    bool inShared = false;
    bool inRequest = false;

    // Go through all sections.
    while (begin != end) {
        // If going forward, the next section informations are in the current iterator.
        // so we wait for the contact first.
        if (!isReverse) {
            attendre_contact(begin->contact);
            loco.afficherMessage("Contact " + QString(std::to_string(begin->contact).c_str()));
        }

        if (inShared && !begin->isShared) {
            // The locomotive leave the shared section.
            inShared = false;
            sharedSection->leave(loco, locoId);
        } else {
            if (!inRequest) {
                auto nextIt = begin + 1;
                if (nextIt != end && nextIt->isShared) {
                    sharedSection->request(loco, locoId, isReverse ? EntryPoint::EB : EntryPoint::EA);
                    inRequest = true;
                }
            } else {
                // The locomotive enter in the shared section.
                inRequest = false;
                inShared = true;
                sharedSection->getAccess(loco, locoId);
            }
        }


        // Change all train switches for needed orientation.
        for (auto itSwitch = begin->railToSwitch.cbegin(); itSwitch != begin->railToSwitch.cend(); ++itSwitch) {
            diriger_aiguillage(itSwitch->first, itSwitch->second, 0);
        }

        // If going backward, the next section informations are in the next iterator,
        // so we wait for the contact before changing of iterator.
        if (isReverse) {
            attendre_contact(begin->contact);
            loco.afficherMessage("Contact " + QString(std::to_string(begin->contact).c_str()));
        }

        ++begin;
    }
}

void LocomotiveBehavior::run()
{
    //Initialisation de la locomotive
    loco.allumerPhares();
    loco.demarrer();
    loco.afficherMessage("Ready!");

    bool reverse = false;
    int nbTurn = 0;
    while (1) {
        // Travel in forward or backward mode.
        if (!reverse) {
            doTravel(travel.cbegin(), travel.cend(), false);
            ++nbTurn;
        } else {
            doTravel(travel.crbegin(), travel.crend(), true);
            ++nbTurn;
        }

        // Change direction after having made all the turn.
        if (nbTurn >= NB_TURN) {
            nbTurn = 0;
            reverse = !reverse;
            loco.inverserSens();
        }
    }
}

void LocomotiveBehavior::printStartMessage()
{
    qDebug() << "[START] Thread de la loco" << loco.numero() << "lancé";
    loco.afficherMessage("Je suis lancée !");
}

void LocomotiveBehavior::printCompletionMessage()
{
    qDebug() << "[STOP] Thread de la loco" << loco.numero() << "a terminé correctement";
    loco.afficherMessage("J'ai terminé");
}
