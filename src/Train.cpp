// handles strings of cars making a train
// includes data for each car
// intent is to correspond with track object to obtain cars and to place cars at various locations on track

#include "Train.h"

Train::Train(void)
{
}

void Train::addCar(char* jsonMsg, uint segmentID)
// add a railcar to the train while simultaneously removing it from a track segment
// this method called from mqtt callback
// The track object would provide a list to the app via json message
// user selects car from list, pushes a button to move to train
// app issues mqtt message containing track segment and car data
// track object responds to that message by removing car from track segment

// app and track server manage the cars database
// only thing this program cares about is the mass of the train
// the track server will send the mass here when changes occur in the db, sent via mqtt message
{
}

void Train::removeCar(char* jsonMsg, uint segmentID)
// like addCar but in reverse - TBD might combine addCar and removeCar into one method
{
}

uint Train::trainMass(void)
{
    return _trainMass;
}

uint Train::carCount(void)
{
    return _carCount;
}

uint Train::trainLength(void)
{
    return _trainLength;
}