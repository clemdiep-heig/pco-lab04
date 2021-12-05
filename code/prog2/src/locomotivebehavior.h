//    ___  _________    ___  ___  ___   __ //
//   / _ \/ ___/ __ \  |_  |/ _ \|_  | / / //
//  / ___/ /__/ /_/ / / __// // / __/ / /  //
// /_/   \___/\____/ /____/\___/____//_/   //
//                                         //

#ifndef LOCOMOTIVEBEHAVIOR_H
#define LOCOMOTIVEBEHAVIOR_H

#include "locomotive.h"
#include "launchable.h"
#include "sharedsectioninterface.h"

/**
 * @brief Represent a section of rail with the first contact,
 * the rail that have to be switch for the train to pass
 * and if it's a shared section.
 */
struct Section {
    int contact;
    std::vector<std::pair<int, int>> railToSwitch;
    bool isShared;
};

/**
 * @brief La classe LocomotiveBehavior représente le comportement d'une locomotive
 */
class LocomotiveBehavior : public Launchable
{
    using LocoId = SharedSectionInterface::LocoId;
    using EntryPoint = SharedSectionInterface::EntryPoint;
public:
    /*!
     * \brief locomotiveBehavior Constructeur de la classe
     * \param loco la locomotive dont on représente le comportement
     */
    LocomotiveBehavior(Locomotive& loco, std::shared_ptr<SharedSectionInterface> sharedSection, std::vector<Section> travel, LocoId locoId):
                                                                        loco(loco), sharedSection(sharedSection), travel(travel), locoId(locoId) {
    }

protected:
    /*!
     * \brief run Fonction lancée par le thread, représente le comportement de la locomotive
     */
    void run() override;

    /*!
     * \brief printStartMessage Message affiché lors du démarrage du thread
     */
    void printStartMessage() override;

    /*!
     * \brief printCompletionMessage Message affiché lorsque le thread a terminé
     */
    void printCompletionMessage() override;

    /**
     * @brief loco La locomotive dont on représente le comportement
     */
    Locomotive& loco;

    /**
     * @brief sharedSection Pointeur sur la section partagée
     */
    std::shared_ptr<SharedSectionInterface> sharedSection;


    /**
     * @brief All section of rail where the train go through.
     * This must represent a complete turn on the maquet.
     * The first and the last section must not indicate a shared section.
     */
    std::vector<Section> travel;

    /**
     * @brief The id of the locomotive for the shared section.
     */
    LocoId locoId;

    /**
     * @brief Follow the travel of the train and wait if needed on
     * shared sections.
     *
     * @param begin First section on which the train will go.
     * @param end Last section on which the train will go.
     * @param isReverse Information to know if the train is going backward.
     */
    template<typename iterator>
    void doTravel(iterator begin, iterator end, bool isReverse);
};

#endif // LOCOMOTIVEBEHAVIOR_H
