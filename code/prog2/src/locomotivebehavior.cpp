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
    auto first = begin;

    // Go through all sections.
    while (begin != end) {
        attendre_contact(begin->contact);
        loco.afficherMessage("I passed the contact no. " + QString(std::to_string(begin->contact).c_str()));
        auto current = begin;

        // If the loco goes in reverse mode, the current section information (as the rail switches direction)
        // is in the next section.
        if (isReverse) {
            current = current + 1;
            if (current == end) {
                current = first;
            }
        }

        if (inShared) {
            // The locomotive is currently in a shared section,
            // verify that the next section is not a shared one
            // to leave it.
            if (!current->isShared) {
                // The locomotive leave the shared section.
                inShared = false;
                sharedSection->leave(loco, locoId);
            }
        } else if (inRequest) {
            // The locomotive enter in the shared section.
            inRequest = false;
            inShared = true;
            sharedSection->getAccess(loco, locoId);
        } else {
            // A request is done if the next contact is in a shared section.
            auto nextIt = current + 1;
            nextIt = nextIt == end ? first : nextIt;
            if (nextIt->isShared) {
                // Request the access with current entry point.
                sharedSection->request(loco, locoId, isReverse ? EntryPoint::EB : EntryPoint::EA);
                inRequest = true;
            }
        }


        // Change all train switches for needed orientation.
        for (auto itSwitch = current->railToSwitch.cbegin(); itSwitch != current->railToSwitch.cend(); ++itSwitch) {
            diriger_aiguillage(itSwitch->first, itSwitch->second, 0);
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
        } else {
            doTravel(travel.crbegin(), travel.crend(), true);
        }
        ++nbTurn;

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
