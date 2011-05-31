/////////////////////////////////////////////////////////////////////////////
// Name:            system.cpp
// Purpose:         Stores and renders a system
// Author:          Brad Larsen
// Modified by:
// Created:         Dec 16, 2004
// RCS-ID:
// Copyright:       (c) Brad Larsen
// License:         wxWindows license
/////////////////////////////////////////////////////////////////////////////

#include "system.h"
#include "staff.h"
#include "barline.h"

#include <algorithm>
#include <stdexcept>
#include <functional>

#include "powertabfileheader.h"                     // Needed for file version constants
#include "direction.h"
#include "chordtext.h"
#include "rhythmslash.h"
#include "position.h"

#include "powertabinputstream.h"
#include "powertaboutputstream.h"

#include <boost/foreach.hpp>

// Default Constants
const Rect System::DEFAULT_RECT                                   = Rect(50, 20, 750, 0);
const uint8_t System::DEFAULT_POSITION_SPACING                       = 20;
const uint8_t System::DEFAULT_RHYTHM_SLASH_SPACING_ABOVE             = 0;
const uint8_t System::DEFAULT_RHYTHM_SLASH_SPACING_BELOW             = 0;
const uint8_t System::DEFAULT_EXTRA_SPACING                          = 0;
const uint8_t System::SYSTEM_SYMBOL_SPACING = 18; // spacing given to a system symbol (i.e. rehearsal sign)
const uint8_t System::RHYTHM_SLASH_SPACING = 2 * System::SYSTEM_SYMBOL_SPACING;

// Position Spacing Constants
const uint8_t System::MIN_POSITION_SPACING                           = 3;

// Constructor/Destructor
/// Default Constructor
System::System() :
    m_rect(DEFAULT_RECT), m_positionSpacing(DEFAULT_POSITION_SPACING),
    m_rhythmSlashSpacingAbove(DEFAULT_RHYTHM_SLASH_SPACING_ABOVE),
    m_rhythmSlashSpacingBelow(DEFAULT_RHYTHM_SLASH_SPACING_BELOW),
    m_extraSpacing(DEFAULT_EXTRA_SPACING),
    m_startBar(new Barline),
    m_endBar(new Barline)
{
    //------Last Checked------//
    // - Jan 14, 2005
}

/// Copy Constructor
System::System(const System& system) :
    m_rect(DEFAULT_RECT), m_positionSpacing(DEFAULT_POSITION_SPACING),
    m_rhythmSlashSpacingAbove(DEFAULT_RHYTHM_SLASH_SPACING_ABOVE),
    m_rhythmSlashSpacingBelow(DEFAULT_RHYTHM_SLASH_SPACING_BELOW),
    m_extraSpacing(DEFAULT_EXTRA_SPACING),
    m_startBar(new Barline),
    m_endBar(new Barline)
{
    //------Last Checked------//
    // - Dec 16, 2004
    *this = system;
}

/// Assignment Operator
const System& System::operator=(const System& system)
{
    //------Last Checked------//
    // - Jan 14, 2005

    // Check for assignment to self
    if (this != &system)
    {
        m_rect = system.m_rect;
        m_positionSpacing = system.m_positionSpacing;
        m_rhythmSlashSpacingAbove = system.m_rhythmSlashSpacingAbove;
        m_rhythmSlashSpacingBelow = system.m_rhythmSlashSpacingBelow;
        m_extraSpacing = system.m_extraSpacing;
        m_startBar = system.m_startBar;
        m_directionArray = system.m_directionArray;
        m_chordTextArray = system.m_chordTextArray;
        m_rhythmSlashArray = system.m_rhythmSlashArray;
        m_staffArray = system.m_staffArray;
        m_barlineArray = system.m_barlineArray;
        m_endBar = system.m_endBar;
    }
    return (*this);
}

/// Equality Operator
bool System::operator==(const System& system) const
{
    //------Last Checked------//
    // - Jan 14, 2005
    return (
        (m_rect == system.m_rect) &&
        (m_positionSpacing == system.m_positionSpacing) &&
        (m_rhythmSlashSpacingAbove == system.m_rhythmSlashSpacingAbove) &&
        (m_rhythmSlashSpacingBelow == system.m_rhythmSlashSpacingBelow) &&
        (m_extraSpacing == system.m_extraSpacing) &&
        (m_startBar == system.m_startBar) &&
        (m_directionArray == system.m_directionArray) &&
        (m_chordTextArray == system.m_chordTextArray) &&
        (m_rhythmSlashArray == system.m_rhythmSlashArray) &&
        (m_staffArray == system.m_staffArray) &&
        (m_barlineArray == system.m_barlineArray) &&
        (m_endBar == system.m_endBar)
    );
}

/// Inequality Operator
bool System::operator!=(const System& system) const
{
    //------Last Checked------//
    // - Jan 5, 2005
    return (!operator==(system));
}

// Serialize Functions
/// Performs serialization for the class
/// @param stream Power Tab output stream to serialize to
/// @return True if the object was serialized, false if not
bool System::Serialize(PowerTabOutputStream& stream) const
{
    //------Last Checked------//
    // - Jan 14, 2005
    stream.WriteMFCRect(m_rect);
    CHECK_THAT(stream.CheckState(), false);

    // Note: End bar is stored as a byte; we use Barline class to make it easier
    // for the user
    uint8_t endBar = (uint8_t)((m_endBar->GetType() << 5) |
        (m_endBar->GetRepeatCount()));
    stream << endBar << m_positionSpacing << m_rhythmSlashSpacingAbove <<
        m_rhythmSlashSpacingBelow << m_extraSpacing;
    CHECK_THAT(stream.CheckState(), false);

    m_startBar->Serialize(stream);
    CHECK_THAT(stream.CheckState(), false);

    stream.WriteVector(m_directionArray);
    CHECK_THAT(stream.CheckState(), false);

    stream.WriteVector(m_chordTextArray);
    CHECK_THAT(stream.CheckState(), false);

    stream.WriteVector(m_rhythmSlashArray);
    CHECK_THAT(stream.CheckState(), false);

    stream.WriteVector(m_staffArray);
    CHECK_THAT(stream.CheckState(), false);

    stream.WriteVector(m_barlineArray);
    CHECK_THAT(stream.CheckState(), false);

    return (stream.CheckState());
}

/// Performs deserialization for the class
/// @param stream Power Tab input stream to load from
/// @param version File version
/// @return True if the object was deserialized, false if not
bool System::Deserialize(PowerTabInputStream& stream, uint16_t version)
{
    //------Last Checked------//
    // - Jan 14, 2005

    // Version 1.0 and 1.0.2
    if (version == PowerTabFileHeader::FILEVERSION_1_0 ||
        version == PowerTabFileHeader::FILEVERSION_1_0_2)
    {
        uint8_t key;
        uint16_t endBar;

        stream.ReadMFCRect(m_rect);
        CHECK_THAT(stream.CheckState(), false);

        stream >> key >> endBar >> m_positionSpacing >>
            m_rhythmSlashSpacingAbove >> m_rhythmSlashSpacingBelow >>
            m_extraSpacing;
        CHECK_THAT(stream.CheckState(), false);

        // Update the key signature at start of section (always shown)
        uint8_t keyType = (uint8_t)((key >> 4) & 0xf);
        uint8_t keyAccidentals = (uint8_t)(key & 0xf);

        m_startBar->GetKeySignatureRef().Show();

        // Cancellation
        if (keyType > 2)
            m_startBar->GetKeySignatureRef().SetCancellation();

        keyType = (uint8_t)(((keyType % 2) == 1) ? KeySignature::majorKey :
            KeySignature::minorKey);

        m_startBar->GetKeySignatureRef().SetKey(keyType, keyAccidentals);

        // Update the ending bar
        uint8_t barType = HIBYTE(endBar);
        uint8_t repeatCount = LOBYTE(endBar);

        m_endBar->SetBarlineData(barType, repeatCount);
        //SetEndBar(barType, repeatCount);

        stream.ReadVector(m_directionArray, version);
        CHECK_THAT(stream.CheckState(), false);

        stream.ReadVector(m_chordTextArray, version);
        CHECK_THAT(stream.CheckState(), false);

        stream.ReadVector(m_rhythmSlashArray, version);
        CHECK_THAT(stream.CheckState(), false);

        stream.ReadVector(m_staffArray, version);
        CHECK_THAT(stream.CheckState(), false);

        stream.ReadVector(m_barlineArray, version);
        CHECK_THAT(stream.CheckState(), false);

        // Any barline at position zero is now stored in the section m_startBar
        if (GetBarlineCount() > 0)
        {
            BarlinePtr barline = m_barlineArray[0];
            if (barline)
            {
                if (barline->GetPosition() == 0)
                {
                    *m_startBar = *barline;
                    m_barlineArray.erase(m_barlineArray.begin());
                }
            }
        }

        // Update key signs that aren't show to match active key sign
        const KeySignature* activeKeySignature = m_startBar->GetKeySignaturePtr();

        size_t i = 0;
        size_t count = m_barlineArray.size();
        for (; i < count; i++)
        {
            KeySignature& keySignature = m_barlineArray[i]->GetKeySignatureRef();

            // Key on bar doesn't match active
            if (keySignature != *activeKeySignature)
            {
                // Key isn't shown, update key to match
                if (!keySignature.IsShown())
                {
                    keySignature = *activeKeySignature;
                    keySignature.Hide();
                    keySignature.SetCancellation(false);
                }

                // Update active key
                activeKeySignature = m_barlineArray[i]->GetKeySignaturePtr();
            }
        }
    }
    // Version 1.5 and up
    else
    {
        stream.ReadMFCRect(m_rect);
        CHECK_THAT(stream.CheckState(), false);

        uint8_t endBar = 0;
        stream >> endBar >> m_positionSpacing >> m_rhythmSlashSpacingAbove >>
            m_rhythmSlashSpacingBelow >> m_extraSpacing;
        CHECK_THAT(stream.CheckState(), false);

        // Update end bar (using Barline class is easier to use)
        m_endBar->SetBarlineData((uint8_t)((endBar & 0xe0) >> 5),
            (uint8_t)(endBar & 0x1f));

        m_startBar->Deserialize(stream, version);
        CHECK_THAT(stream.CheckState(), false);

        stream.ReadVector(m_directionArray, version);
        CHECK_THAT(stream.CheckState(), false);

        stream.ReadVector(m_chordTextArray, version);
        CHECK_THAT(stream.CheckState(), false);

        stream.ReadVector(m_rhythmSlashArray, version);
        CHECK_THAT(stream.CheckState(), false);

        stream.ReadVector(m_staffArray, version);
        CHECK_THAT(stream.CheckState(), false);

        stream.ReadVector(m_barlineArray, version);
        CHECK_THAT(stream.CheckState(), false);
    }

    return (stream.CheckState());
}

// Barline functions
/// Gets the bar at the start of the system
/// @return The start bar
System::BarlinePtr System::GetStartBar() const
{
    return m_startBar;
}

/// Determines if a barline index is valid
/// @param index barline index to validate
/// @return True if the barline index is valid, false if not
bool System::IsValidBarlineIndex(uint32_t index) const
{
    return index < GetBarlineCount();
}

/// Gets the number of barlines in the system
/// @return The number of barlines in the system
size_t System::GetBarlineCount() const
{
    return m_barlineArray.size();
}

/// Gets the nth barline in the system
/// @param index Index of the barline to get
/// @return The nth barline in the system
System::BarlinePtr System::GetBarline(uint32_t index) const
{
    CHECK_THAT(IsValidBarlineIndex(index), BarlinePtr());
    return m_barlineArray[index];
}

/// Gets the bar at the end of the system
/// @return The end bar
System::BarlinePtr System::GetEndBar() const
{
    return m_endBar;
}

// Barline Array Functions
/// Gets the barline at a given position
/// @param position Position to get the barline for
/// @return A pointer to the barline at the position, or NULL if the barline
/// doesn't exist
System::BarlinePtr System::GetBarlineAtPosition(uint32_t position) const
{
    // start bar
    if (position == 0)
    {
        return m_startBar;
    }

    // Iterate through the barlines
    for (size_t barlineIndex = 0; barlineIndex < m_barlineArray.size(); barlineIndex++)
    {
        BarlinePtr barline = GetBarline(barlineIndex);

        // Found it; return the barline
        if (barline->GetPosition() == position)
            return barline;
    }

    // last bar of system
    if (position == static_cast<uint32_t>(GetPositionCount()))
    {
        return m_endBar;
    }

    // Barline not found at position
    return BarlinePtr();
}

// Comparison functor for barline positions
struct CompareBarlineToPosition
{
    uint32_t position;
    bool operator()(const System::BarlineConstPtr& barline)
    {
        return barline->GetPosition() <= position;
    }
};

// Barline Array Functions
/// Gets the barline preceding a given position
/// @param position Position to get the preceding barline for
/// @return A pointer to the barline preceding the position
System::BarlinePtr System::GetPrecedingBarline(uint32_t position) const
{
    if (m_barlineArray.empty() || position < m_barlineArray.at(0)->GetPosition())
    {
        return m_startBar;
    }

    CompareBarlineToPosition compareToPosition;
    compareToPosition.position = position;
    auto barline = std::find_if(m_barlineArray.rbegin(), m_barlineArray.rend(), compareToPosition);

    return *barline;
}

System::BarlinePtr System::GetNextBarline(uint32_t position) const
{
    // if position is past the last barline in array return m_endBar
    if (m_barlineArray.empty() ||
        position >= m_barlineArray.at(GetBarlineCount() - 1)->GetPosition())
    {
        return m_endBar;
    }

    // if position is before the first non-startbar barline
    if (position < m_barlineArray.at(0)->GetPosition())
        return m_barlineArray.at(0);
    
    CompareBarlineToPosition compareToPosition;
    compareToPosition.position = position;
    auto barline = std::find_if(m_barlineArray.rbegin(), m_barlineArray.rend(),
                                compareToPosition);

    --barline;
    return *barline;
}

/// Gets a list of barlines in the system
/// @param barlineArray Holds the barline return values
void System::GetBarlines(std::vector<BarlineConstPtr>& barlineArray) const
{
    barlineArray.clear();
    barlineArray.push_back(m_startBar);

    for (int barline = 0, barlineCount = m_barlineArray.size();
        barline < barlineCount; ++barline)
    {
        barlineArray.push_back(m_barlineArray[barline]);
    }

    barlineArray.push_back(m_endBar);
}

// Position Functions
/// Determines if a position is valid
/// @param position Zero-based index of the position to validate
/// @return True if the position is valid, false if not
bool System::IsValidPosition(int position) const
{
    //------Last Checked------//
    // - Aug 30, 2007
    return ((position >= 0) && (position <= GetPositionCount()));
}

/// Calculates the number of positions that will fit across the system based on
/// a given position spacing
/// @param positionSpacing Position spacing used to perform the calculation
/// @return The number of positions that will fit across the system
int System::CalculatePositionCount(int positionSpacing) const
{
    if (positionSpacing < MIN_POSITION_SPACING)
        return 0;

    int returnValue = 0;

    // The available width extends from the first position to the right side of
    // the system
    int width = m_rect.GetWidth();

    // Subtract the width of the key and time signatures on the barlines within
    // the system (does not include the starting barline)
    width -= GetCumulativeInternalKeyAndTimeSignatureWidth();

    // We need at least 1 position worth of space from the last position and the
    // end of the system
    width -= positionSpacing;

    // If we have enough width for at least 1 position, calculate the position
    // count
    if (width >= positionSpacing)
        returnValue = width / positionSpacing;

    return (returnValue);
}

/// Gets the number of positions that can fit across the system based on the
/// current position width
/// @return The number of positions that can fit across the system
int System::GetPositionCount() const
{
    //------Last Checked------//
    // - Aug 30, 2007

    // Calculate the position count using the current position spacing
    return (CalculatePositionCount(GetPositionSpacing()));
}

/// Gets the x co-ordinate of the first position in the system, relative to the left edge
/// @return The x co-ordinate of the first position in the system
int System::GetFirstPositionX() const
{
    //------Last Checked------//
    // - Aug 30, 2007

    int returnValue = 0;

    // Add the width of the clef; the symbol itself is 16 units wide, with 3
    // units of space on both sides, for a total of 22 units
    returnValue += 22;

    // Add the width of the starting key signature
    int keySignatureWidth = m_startBar->GetKeySignatureConstRef().GetWidth();
    returnValue += keySignatureWidth;

    // Add the width of the starting time signature
    int timeSignatureWidth = m_startBar->GetTimeSignatureConstRef().GetWidth();
    returnValue += timeSignatureWidth;

    // If we have both a key and time signature, they are separated by 3 units
    if ((keySignatureWidth > 0) && (timeSignatureWidth > 0))
        returnValue += 3;

    // Add the width required by the starting barline; for a standard barline,
    // this is 1 unit of space, otherwise it is the distance between positions
    int barlineWidth = ((m_startBar->IsBar()) ? 1 : GetPositionSpacing());
    returnValue += barlineWidth;

    return (returnValue);
}

/// Gets the x co-ordinate of the nth position in the system, relative to the left edge of the system
/// @param position Zero-based index of the position to retrieve the x
/// co-ordinate for
/// @return The x co-ordinate for the position, or the x co-ordinate of the
/// first position if the position is invalid
int System::GetPositionX(int position) const
{
    //------Last Checked------//
    // - Aug 30, 2007

    // Initialize to the first position
    int returnValue = GetFirstPositionX();

    // Validate the position
    CHECK_THAT(IsValidPosition(position), returnValue);

    // Get the width of all key and time signatures up to, but not
    // including, the position
    int keyAndTimeSignatureWidth =
            GetCumulativeInternalKeyAndTimeSignatureWidth(position);

    // Move "n" positions across using the position spacing, adding the
    // cumulative key and time signature widths. Add 1 since the position
    // value is zero-based
    returnValue += (((position + 1) * GetPositionSpacing()) +
                    keyAndTimeSignatureWidth);

    return (returnValue);
}

/// Gets the position index for an x-coordinate in the system
/// @return The closest position to the given x-coordinate, or the first/last position if it is out of range
size_t System::GetPositionFromX(int x) const
{
    if (GetPositionX(0) >= x)
    {
        return 0;
    }

    for (int i = 1; i < GetPositionCount(); i++)
    {
        if (GetPositionX(i) >= x)
        {
            return i - 1;
        }
    }

    // if the x-coordinate is past the last position, just return the last position index
    return GetPositionCount() - 1;
}

// Operations
/// Gets the total width used by all key and time signatures that reside within
/// the system (does not include the start bar)
/// @param position Zero-based index of the position to stop at. If -1, traverse
/// all the barlines
int System::GetCumulativeInternalKeyAndTimeSignatureWidth(int position) const
{
    //------Last Checked------//
    // - Sep 01, 2007
    int returnValue = 0;

    bool bAllBarlines = (position == -1);

    // Loop through barline list
    for (int barline = 0, barlineCount = m_barlineArray.size();
        barline < barlineCount; ++barline)
    {
        // Get the position where the barline resides
        int barlinePosition = m_barlineArray[barline]->GetPosition();

        // Only use bars before the index
        if (bAllBarlines || (barlinePosition < position))
        {
            // Ignore keys and time signs at position 0, they're handled in
            // GetFirstPositionX
            if (m_barlineArray[barline]->GetPosition() > 0)
            {
                // Add the width of the key and time signature, if present on
                // the barline
                returnValue +=
                    m_barlineArray[barline]->GetKeyAndTimeSignatureWidth();
            }
        }
        else
            break;
    }

    return (returnValue);
}

// Calculate the height of the entire system
void System::CalculateHeight()
{
    // get height offset to the top of the last staff
    int sum = GetStaffHeightOffset(m_staffArray.size() - 1);

    // add the height of the last staff
    sum += m_staffArray.at(m_staffArray.size() - 1)->GetHeight();

    m_rect.SetHeight(sum);
}

// Get the height offset of a staff from the top of the system
// Optionally, can return the absolute position of the top of the staff
uint32_t System::GetStaffHeightOffset(uint32_t staff, bool absolutePos) const
{
    CHECK_THAT(IsValidStaffIndex(staff), 0);

    uint32_t offset = GetExtraSpacing() + GetRhythmSlashSpacingAbove() + GetRhythmSlashSpacingBelow();

    if (GetRhythmSlashCount() != 0)
    {
        offset += RHYTHM_SLASH_SPACING;
    }

    for (uint32_t i = 0; i < staff; i++)
    {
        offset += m_staffArray.at(i)->GetHeight();
    }

    if (absolutePos)
    {
        offset += GetRect().GetTop();
    }

    return offset;
}

/// Determines if a chord text index is valid
/// @param index chord text index to validate
/// @return True if the chord text index is valid, false if not
bool System::IsValidChordTextIndex(uint32_t index) const
{
    return index < GetChordTextCount();
}

/// Gets the number of chord text items in the system
/// @return The number of chord text items in the system
size_t System::GetChordTextCount() const
{
    return m_chordTextArray.size();
}

/// Gets the nth chord text item in the system
/// @param index Index of the chord text to get
/// @return The nth chord text item in the system
System::ChordTextPtr System::GetChordText(uint32_t index) const
{
    CHECK_THAT(IsValidChordTextIndex(index), ChordTextPtr());
    return m_chordTextArray[index];
}

/// @return true If a ChordText item exists at the given position
bool System::HasChordText(uint32_t position) const
{
    return FindChordText(position) != -1;
}

/// Searches for a ChordText object with the specified position
/// @return The index of that object if found, and returns -1 otherwise
int System::FindChordText(uint32_t position) const
{
    CHECK_THAT(IsValidPosition(position), -1);

    for (uint32_t i = 0; i < m_chordTextArray.size(); i++)
    {
        if (m_chordTextArray.at(i)->GetPosition() == position)
        {
            return i;
        }
    }

    return -1;
}

/// Inserts a ChordText object at the specified index
/// @return true if the insertion succeeded
bool System::InsertChordText(ChordTextPtr chordText, uint32_t index)
{
    CHECK_THAT(index <= GetChordTextCount(), false);

    m_chordTextArray.insert(m_chordTextArray.begin() + index, chordText);
    return true;
}

/// Removes a ChordText object at the specified index
/// @return true if the removal succeeded
bool System::RemoveChordText(uint32_t index)
{
    CHECK_THAT(IsValidChordTextIndex(index), false);

    m_chordTextArray.erase(m_chordTextArray.begin() + index);
    return true;
}

// Staff Functions
/// Determines if a staff index is valid
/// @param index staff index to validate
/// @return True if the staff index is valid, false if not
bool System::IsValidStaffIndex(uint32_t index) const
{
    return index < GetStaffCount();
}

/// Gets the number of staffs in the system
/// @return The number of staffs in the system
size_t System::GetStaffCount() const
{
    return m_staffArray.size();
}

/// Gets the nth staff in the system
/// @param index Index of the staff to get
/// @return The nth staff in the system
System::StaffPtr System::GetStaff(uint32_t index) const
{
    CHECK_THAT(IsValidStaffIndex(index), StaffPtr());
    return m_staffArray[index];
}

/// Returns the index of a staff within the system
/// @param staff The staff to be located in the system
/// @return The zero-based index of the staff
/// @throw std::out_of_range if the staff does not exist in the system
size_t System::FindStaffIndex(StaffConstPtr staff) const
{
    auto result = std::find(m_staffArray.begin(), m_staffArray.end(), staff);

    if (result == m_staffArray.end())
        throw std::out_of_range("Staff not in system");

    return std::distance(m_staffArray.begin(), result);
}

// Checks if a rehearsal sign occurs in the system
bool System::HasRehearsalSign() const
{
    if (m_startBar->GetRehearsalSignConstRef().IsSet() || m_endBar->GetRehearsalSignConstRef().IsSet())
    {
        return true;
    }

    for (auto i = m_barlineArray.begin(); i != m_barlineArray.end(); ++i)
    {
        if ((*i)->GetRehearsalSignConstRef().IsSet())
        {
            return true;
        }
    }
    return false;
}

/// Recalculates the note beaming for each staff in the system
void System::CalculateBeamingForStaves()
{
    std::vector<BarlineConstPtr> barlines;
    GetBarlines(barlines);

    // the end bar doesn't keep track of its position normally, so add it in for these calculations
    m_endBar->SetPosition(this->GetPositionCount());

    for(auto staff = m_staffArray.begin(); staff != m_staffArray.end(); ++staff)
    {
        // calculate the beaming for the notes between each pair of barlines
        for (size_t i = 0; i < barlines.size() - 1; i++)
        {
            (*staff)->CalculateBeamingForBar(barlines.at(i), barlines.at(i + 1));
        }
    }
}

/// Determines if a position spacing is valid
/// @param positionSpacing Position spacing to validate
/// @return True if the position spacing is valid, false if not
bool System::IsValidPositionSpacing(int positionSpacing) const
{
    if (positionSpacing < MIN_POSITION_SPACING)
        return false;

    // find the max number of positions that will fit using the given spacing
    const uint32_t maxNumPositions = CalculatePositionCount(positionSpacing);

    // need to compare against the positions in each member staff
    for (size_t i = 0; i < m_staffArray.size(); i++)
    {
        Position* lastPositionInStaff = m_staffArray.at(i)->GetLastPosition();
        if (lastPositionInStaff != NULL && lastPositionInStaff->GetPosition() >= maxNumPositions - 1)
        {
            return false;
        }
    }

    return true;
}

/// Sets the position spacing for the system
bool System::SetPositionSpacing(uint8_t positionSpacing)
{
    if (!IsValidPositionSpacing(positionSpacing))
        return false;

    m_positionSpacing = positionSpacing;

    return true;
}

/// Shifts the position for an object (must provide GetPosition() and SetPosition(uint32_t) functions)
/// The object is constructed with a position comparison function, a position index, and an offset.
/// The object's position is compared with the supplied index using the given function, and is adjusted by the offset
/// if the comparison function is satisfied.
template <class T>
struct ShiftPosition
{
    typedef std::function<bool (uint32_t, uint32_t)> PositionIndexComparison;

    ShiftPosition(PositionIndexComparison comparePositions, uint32_t positionIndex, int offset) :
        comparePositions(comparePositions),
        positionIndex(positionIndex),
        offset(offset)
    {
    }

    void operator()(const T& item)
    {
        if (comparePositions(item->GetPosition(), positionIndex))
        {
            item->SetPosition(item->GetPosition() + offset);
        }
    }

    PositionIndexComparison comparePositions;
    uint32_t positionIndex;
    int offset;
};

/// Shifts all positions forward/backward starting from the given index.
void System::PerformPositionShift(uint32_t positionIndex, int offset)
{
    if (!IsValidPosition(positionIndex))
        throw std::out_of_range("Invalid position index");

    const std::function<bool (uint32_t, uint32_t)> comparison = std::greater_equal<uint32_t>();

    // decrease the position spacing to make room for the extra position
    SetPositionSpacing(GetPositionSpacing() - offset);

    // shift forward barlines
    ShiftPosition<BarlinePtr> shiftBarlines(comparison, positionIndex, offset);
    std::for_each(m_barlineArray.begin(), m_barlineArray.end(), shiftBarlines);

    // shift direction symbols
    ShiftPosition<DirectionPtr> shiftDirections(comparison, positionIndex, offset);
    std::for_each(m_directionArray.begin(), m_directionArray.end(), shiftDirections);

    // shift chords
    ShiftPosition<ChordTextPtr> shiftChords(comparison, positionIndex, offset);
    std::for_each(m_chordTextArray.begin(), m_chordTextArray.end(), shiftChords);

    // shift rhythm slashes
    ShiftPosition<RhythmSlashPtr> shiftSlashes(comparison, positionIndex, offset);
    std::for_each(m_rhythmSlashArray.begin(), m_rhythmSlashArray.end(), shiftSlashes);

    // shift positions for each staff
    ShiftPosition<Position*> shiftPosition(comparison, positionIndex, offset);

    for (size_t i = 0; i < m_staffArray.size(); i++)
    {
        StaffConstPtr staff = m_staffArray.at(i);

        for (size_t j = 0; j < staff->GetPositionCount(0); j++)
        {
            shiftPosition(staff->GetPosition(0, j));
        }
    }
}

void System::ShiftForward(uint32_t positionIndex)
{
    PerformPositionShift(positionIndex, 1);
}

void System::ShiftBackward(uint32_t positionIndex)
{
    PerformPositionShift(positionIndex, -1);
}

/// Initializes the system, creating staves using the given list of staff sizes (# of strings)
void System::Init(const std::vector<uint8_t>& staffSizes)
{
    m_staffArray.clear();

    for (size_t i = 0; i < staffSizes.size(); i++)
    {
        m_staffArray.push_back(std::make_shared<Staff>(staffSizes.at(i), Staff::TREBLE_CLEF));
    }

    CalculateHeight();
}

/// Removes the barline at the given position, if possible
bool System::RemoveBarline(uint32_t position)
{
    using std::bind;
    using namespace std::placeholders;

    // find the barline that has the given position
    auto bar = std::find_if(m_barlineArray.begin(), m_barlineArray.end(),
                            bind(std::equal_to<uint32_t>(),
                                 bind(&Barline::GetPosition, _1), position)
                            );

    if (bar == m_barlineArray.end())
        return false;

    // remove the bar from the array
    m_barlineArray.erase(bar);
    return true;
}

namespace
{
struct CompareBarlinesByPosition
{
    bool operator() (const System::BarlineConstPtr& bar1,
                     const System::BarlineConstPtr& bar2)
    {
        return bar1->GetPosition() < bar2->GetPosition();
    }
};
}

/// Inserts the given barline.
/// The barline array is then sorted by position
bool System::InsertBarline(BarlinePtr barline)
{
    m_barlineArray.push_back(barline);
    std::sort(m_barlineArray.begin(), m_barlineArray.end(), CompareBarlinesByPosition());

    return true;
}

// Direction Functions
/// Determines if a staff index is valid
/// @param index staff index to validate
/// @return True if the staff index is valid, false if not
bool System::IsValidDirectionIndex(uint32_t index) const
{
    return index < GetDirectionCount();
}

/// Gets the number of staffs in the system
/// @return The number of staffs in the system
size_t System::GetDirectionCount() const
{
    return m_directionArray.size();
}

/// Gets the nth staff in the system
/// @param index Index of the staff to get
/// @return The nth staff in the system
System::DirectionPtr System::GetDirection(uint32_t index) const
{
    CHECK_THAT(IsValidDirectionIndex(index), DirectionPtr());
    return m_directionArray[index];
}

/// Returns the largest number of symbols used by a Direction in the system
size_t System::MaxDirectionSymbolCount() const
{
    if (m_directionArray.empty())
    {
        return 0;
    }

    std::vector<size_t> directionCounts(m_directionArray.size());

    using namespace std::placeholders;
    std::transform(m_directionArray.begin(), m_directionArray.end(), directionCounts.begin(),
                   std::bind(&Direction::GetSymbolCount, _1));

    return *std::max_element(directionCounts.begin(), directionCounts.end());
}

// Rhythm Slash Functions
/// Determines if a rhythm slash index is valid
/// @param index rhythm slash index to validate
/// @return True if the rhythm slash index is valid, false if not
bool System::IsValidRhythmSlashIndex(uint32_t index) const
{
    return index < GetRhythmSlashCount();
}

/// Gets the number of rhythm slashes in the system
/// @return The number of rhythm slashes in the system
size_t System::GetRhythmSlashCount() const
{
    return m_rhythmSlashArray.size();
}

/// Gets the nth rhythm slash in the system
/// @param index Index of the rhythm slash to get
/// @return The nth rhythm slash in the system
System::RhythmSlashPtr System::GetRhythmSlash(uint32_t index) const
{
    CHECK_THAT(IsValidRhythmSlashIndex(index), RhythmSlashPtr());
    return m_rhythmSlashArray[index];
}

/// Searches for a multi-bar rest in the given bar
/// @param measureCount - output parameter to store the measure count for the multi-bar rest
bool System::HasMultiBarRest(BarlineConstPtr startBar, uint8_t& measureCount) const
{
    measureCount = 1;
    BarlineConstPtr nextBar = GetNextBarline(startBar->GetPosition());

    // search through all positions in the bar, for each voice in each staff
    BOOST_FOREACH(StaffPtr staff, m_staffArray)
    {
        for (uint32_t voice = 0; voice < Staff::NUM_STAFF_VOICES; voice++)
        {
            std::vector<Position*> positions;
            staff->GetPositionsInRange(positions, 0, startBar->GetPosition(),
                                       nextBar->GetPosition());

            BOOST_FOREACH(const Position* pos, positions)
            {
                if (pos->HasMultibarRest())
                {
                    pos->GetMultibarRest(measureCount);
                    return true;
                }
            }
        }
    }

    return false;
}
