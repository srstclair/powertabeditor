#include "caret.h"

#include <QPainter>
#include <QVector>
#include <QDebug>

#include <powertabdocument/score.h>
#include <powertabdocument/system.h>
#include <powertabdocument/staff.h>
#include <powertabdocument/note.h>
#include <powertabdocument/barline.h>
#include <powertabdocument/position.h>

#include <boost/format.hpp>
#include <algorithm>

using std::shared_ptr;
using std::vector;

Caret::Caret(int tabLineSpacing) :
    inPlaybackMode(false),
    currentScore(NULL),
    currentSystemIndex(0),
    currentStaffIndex(0),
    currentStringIndex(0),
    currentPositionIndex(0),
    currentVoice(0),
    currentStaffTopEdge(0),
    lineSpacing(tabLineSpacing),
    selectionRange(std::make_pair(0, 0))
{
}

void Caret::setPlaybackMode(bool playBack)
{
    inPlaybackMode = playBack;
    update(boundingRect()); // redraw the caret
}

void Caret::setScore(Score* newScore)
{
    currentScore = newScore;
}

Score* Caret::getCurrentScore() const
{
    return currentScore;
}

QRectF Caret::boundingRect() const
{
    return QRectF(0, -CARET_NOTE_SPACING, getCurrentSystem()->GetRect().GetWidth(),
                  currentStaffInfo.getTabStaffSize() + 2 * CARET_NOTE_SPACING);
}

void Caret::updateStaffInfo()
{
    shared_ptr<const Staff> currentStaff = getCurrentStaff();
    shared_ptr<const System> currentSystem = getCurrentSystem();

    currentStaffInfo.numOfStrings = currentStaff->GetTablatureStaffType();
    currentStaffInfo.stdNotationStaffAboveSpacing = currentStaff->GetStandardNotationStaffAboveSpacing();
    currentStaffInfo.stdNotationStaffBelowSpacing = currentStaff->GetStandardNotationStaffBelowSpacing();
    currentStaffInfo.symbolSpacing = currentStaff->GetSymbolSpacing();
    currentStaffInfo.tabLineSpacing = lineSpacing;
    currentStaffInfo.tabStaffBelowSpacing = currentStaff->GetTablatureStaffBelowSpacing();
    currentStaffInfo.topEdge = currentSystem->GetRect().GetTop();
    currentStaffInfo.leftEdge = currentSystem->GetRect().GetLeft();
    currentStaffInfo.width = currentSystem->GetRect().GetWidth();
    currentStaffInfo.positionWidth = currentSystem->GetPositionSpacing();
    currentStaffInfo.calculateHeight();
}


void Caret::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    updateStaffInfo();

    const double PEN_WIDTH = 0.75;
    // set color
    if (inPlaybackMode)
    {
        painter->setPen(QPen(Qt::red, PEN_WIDTH));
    }
    else
    {
        painter->setPen(QPen(Qt::blue, PEN_WIDTH));
    }

    shared_ptr<System> currentSystem = getCurrentSystem();
    const int leftPos = currentSystem->GetPositionX(currentPositionIndex);

    // get top
    int y1 = 0;
    // get bottom
    int y2 = currentStaffInfo.getTabStaffSize();
    // get x-coordinate
    int x = centerItem(leftPos, leftPos + currentStaffInfo.positionWidth, 1);

    if (inPlaybackMode) // if in playback mode, just draw a vertical line and don't highlight selected note
    {
        painter->drawLine(x, y1, x, y2);
    }
    else
    {
        QVector<QLine> lines(0);
        // calculations for the box around the selected note
        int stringHeight = currentStaffInfo.getTabLineHeight(currentStringIndex + 1) - currentStaffInfo.getTopTabLine();
        int boundary1 = stringHeight - CARET_NOTE_SPACING;
        int boundary2 = stringHeight + CARET_NOTE_SPACING;

        // draw a line down to the highlighted string, if necessary
        if (y1 < boundary1)
        {
            lines.append(QLine(x, y1, x, boundary1));
        }

        // draw horizontal lines around note
        lines.append(QLine(leftPos, boundary1, leftPos + currentStaffInfo.positionWidth, boundary1));
        lines.append(QLine(leftPos, boundary2, leftPos + currentStaffInfo.positionWidth, boundary2));

        // draw to bottom of staff, if necessary
        if (y2 > boundary2)
        {
            lines.append(QLine(x, boundary2, x, y2));
        }
        painter->drawLines(lines);
    }

    paintSelection(painter);
}

/// Draws a highlighted box around the selected range
void Caret::paintSelection(QPainter* painter)
{
    painter->setBrush(QColor(168, 205, 241, 125));
    painter->setPen(QPen(QBrush(), 0));

    shared_ptr<System> system = getCurrentSystem();

    int leftPos = system->GetPositionX(std::min(selectionRange.first, selectionRange.second));
    int rightPos = system->GetPositionX(std::max(selectionRange.first, selectionRange.second));

    if (selectionRange.first != selectionRange.second)
    {
        rightPos += currentStaffInfo.positionWidth;
    }

    QRectF rect(leftPos, 1, rightPos - leftPos, currentStaffInfo.getTabStaffSize() - 1);
    painter->drawRect(rect);
}

void Caret::updatePosition()
{
    shared_ptr<const System> currentSystem = getCurrentSystem();

    updateStaffInfo();
    setPos(currentSystem->GetRect().GetLeft(),
           currentSystem->GetStaffHeightOffset(currentStaffIndex, true) + currentStaffInfo.getTabStaffOffset());
    emit moved();

    // useful debug information - display the caret's location in the widget's tooltip
    boost::format output = boost::format("System: %d, Staff: %d, Position: %d, String: %d") % currentSystemIndex
            % currentStaffIndex % currentPositionIndex % currentStringIndex;

    setToolTip(QString::fromStdString(output.str()));
}

// Moves the caret either left or right
bool Caret::moveCaretHorizontal(int offset)
{
    const quint32 nextPosition = currentPositionIndex + offset;
    if (getCurrentSystem()->IsValidPosition(nextPosition)) // check that the next position is valid
    {
        setCurrentPositionIndex(nextPosition);
        return true;
    }
    return false;
}

bool Caret::setCurrentStringIndex(uint8_t stringIndex)
{
    if (stringIndex < currentStaffInfo.numOfStrings)
    {
        currentStringIndex = stringIndex;
        update(boundingRect());
        emit moved();
        return true;
    }
    else
    {
        return false;
    }
}

bool Caret::setCurrentSystemIndex(uint32_t systemIndex)
{
    if (currentScore->IsValidSystemIndex(systemIndex))
    {
        currentSystemIndex = systemIndex;
        updatePosition();
        update(boundingRect());
        return true;
    }
    else
    {
        return false;
    }
}

bool Caret::setCurrentStaffIndex(uint32_t staffIndex)
{
    if (getCurrentSystem()->IsValidStaffIndex(staffIndex))
    {
        currentStaffIndex = staffIndex;
        updatePosition();
        update(boundingRect());
        return true;
    }
    else
    {
        return false;
    }
}

bool Caret::setCurrentPositionIndex(uint8_t positionIndex)
{
    if (positionIndex < getCurrentSystem()->GetPositionCount())
    {
        currentPositionIndex = positionIndex;
        selectionRange = std::make_pair(positionIndex, positionIndex);
        updatePosition();
        update(boundingRect());
        return true;
    }
    else
    {
        return false;
    }
}

// Moves the caret up or down
void Caret::moveCaretVertical(int offset)
{
    currentStringIndex = (currentStringIndex + offset + currentStaffInfo.numOfStrings) % currentStaffInfo.numOfStrings;
    update(boundingRect()); // trigger a re-draw
    emit moved();
}

// Moves the caret to the first position in the staff
void Caret::moveCaretToStart()
{
    moveCaretHorizontal( -((int)currentPositionIndex) );
}

// Moves the caret to the last position in the staff
void Caret::moveCaretToEnd()
{
    moveCaretHorizontal( getCurrentSystem()->GetPositionCount() - currentPositionIndex - 1 );
}

bool Caret::moveCaretStaff(int offset)
{
    const quint32 nextStaff = currentStaffIndex + offset;

    if (getCurrentSystem()->IsValidStaffIndex(nextStaff))
    {
        currentStaffIndex = nextStaff;
        currentStringIndex = 0;
        currentPositionIndex = 0;
        selectionRange = std::make_pair(0, 0);

        updatePosition();

        return true;
    }

    return false;
}

bool Caret::moveCaretSection(int offset)
{
    const quint32 nextSystem = currentSystemIndex + offset;
    if (currentScore->IsValidSystemIndex(nextSystem))
    {
        currentStringIndex = 0;
        currentPositionIndex = 0;
        currentStaffIndex = 0;
        selectionRange = std::make_pair(0, 0);
        currentSystemIndex = nextSystem;

        updatePosition();
        return true;
    }
    return false;
}

void Caret::moveCaretToFirstSection()
{
    moveCaretSection( -((int)currentSystemIndex) );
}

void Caret::moveCaretToLastSection()
{
    moveCaretSection( currentScore->GetSystemCount() - currentSystemIndex - 1 );
}

void Caret::moveCaretToNextBar()
{
    System::BarlineConstPtr nextBarline = getCurrentSystem()->GetNextBarline(currentPositionIndex);
    
    // if we are at the end of the system go to next staff
    if (nextBarline == getCurrentSystem()->GetEndBar())
    {
        // if we can't move to the next staff, move to next section
        if (!moveCaretStaff(1))
            moveCaretSection(1);
    }
    else
    {
        // move caret to the first note position of the next bar
        moveCaretHorizontal
                (nextBarline->GetPosition() - currentPositionIndex + 1);
    }
}

void Caret::moveCaretToPrevBar()
{
    shared_ptr<System> currentSystem = getCurrentSystem();
    System::BarlineConstPtr prevBarline = currentSystem->GetPrecedingBarline(currentPositionIndex);

    // if we are at the beginning of the system back up one
    if (prevBarline == currentSystem->GetStartBar())
    {
        // move caret up one staff or section
        if (moveCaretStaff(-1) || moveCaretSection(-1))
        {
            // if new system has > 1 bar, and we actually moved up one
            // (i.e. we are not already at the very first system), move 
            // caret to beginning of last bar
            shared_ptr<const System> newCurrentSystem = getCurrentSystem();
            if (newCurrentSystem->GetBarlineCount() > 0)
            {
                System::BarlineConstPtr lastBar = newCurrentSystem->GetBarline(newCurrentSystem->GetBarlineCount() - 1);

                moveCaretHorizontal(lastBar->GetPosition() - currentPositionIndex + 1);
            }
        }
    }
    else
    {
        // if we need to move to beginning of system
        if (prevBarline == currentSystem->GetBarline(0))
        {
            moveCaretToStart();
            return;
        }
    
        std::vector<System::BarlineConstPtr> sysBars;
        currentSystem->GetBarlines(sysBars);
        
        // we want the position of the bar line before this one
        auto barlinesIt = std::find(sysBars.begin(), sysBars.end(), prevBarline);
        barlinesIt--;
        
        moveCaretHorizontal
                (((int) (*barlinesIt)->GetPosition()) - 
                 ((int) currentPositionIndex) + 1);
    }
}

shared_ptr<System> Caret::getCurrentSystem() const
{
    return currentScore->GetSystem(currentSystemIndex);
}

shared_ptr<Staff> Caret::getCurrentStaff() const
{
    return getCurrentSystem()->GetStaff(currentStaffIndex);
}

/// may return NULL if the current position does not have a Position object
Position* Caret::getCurrentPosition() const
{
    return getCurrentStaff()->GetPositionByPosition(currentVoice, currentPositionIndex);
}

Note* Caret::getCurrentNote() const
{
    Position* currentPosition = getCurrentPosition();
    if (currentPosition != NULL)
    {
        return currentPosition->GetNoteByString(currentStringIndex);
    }
    else
    {
        return NULL;
    }
}

/// Returns the barline at the current position (or NULL if the current position is not a barline)
std::shared_ptr<Barline> Caret::getCurrentBarline() const
{
    return getCurrentSystem()->GetBarlineAtPosition(currentPositionIndex);
}

void Caret::updateSelection(int start, int end)
{
    qDebug() << "Selected Range: " << start << ", " << end;
    selectionRange = std::make_pair(start, end);
}

/// Returns a list of all of the Position objects that are currently selected
std::vector<Position*> Caret::getSelectedPositions() const
{
    std::vector<Position*> positions;
    getCurrentStaff()->GetPositionsInRange(positions, 0,
                                           std::min(selectionRange.first, selectionRange.second),
                                           std::max(selectionRange.first, selectionRange.second));

    return positions;
}

/// Returns a list of all of the Note objects that are currently selected
std::vector<Note*> Caret::getSelectedNotes() const
{
    const std::vector<Position*> positions = getSelectedPositions();

    std::vector<Note*> notes;

    for (size_t i = 0; i < positions.size(); i++)
    {
        for (size_t j = 0; j < positions[i]->GetNoteCount(); j++)
        {
            notes.push_back(positions[i]->GetNote(j));
        }
    }

    return notes;
}

/// Returns the index of the voice that is currently active
uint32_t Caret::getCurrentVoice() const
{
    return currentVoice;
}

bool Caret::setCurrentVoice(uint32_t voice)
{
    CHECK_THAT(Staff::IsValidVoice(voice), false);

    currentVoice = voice;
    return true;
}
