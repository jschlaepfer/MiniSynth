/*
    File: PresetManager.cpp
    Project: MiniSynth
    Revision: 1.0.0
    Date: 2025-08-19
    Description:
        Preset manager implementation. Scans BinaryData and user directory,
        loads JSON (with optional meta) or XML states, and saves/deletes user presets.
*/

#include "PresetManager.h"

#if __has_include(<BinaryData.h>)
 #include <BinaryData.h>
 #define MS_HAVE_BINARYDATA 1
#else
 #define MS_HAVE_BINARYDATA 0
#endif

using namespace juce;

namespace presets {

PresetManager::PresetManager (AudioProcessorValueTreeState& s, String org, String app)
: apvts (s), organisation (std::move (org)), application (std::move (app))
{
    rebuildList();
}

static bool isPresetResource (const char* name) {
    auto str = String (name);
    return str.endsWithIgnoreCase (".minisynth.json");
}

void PresetManager::rebuildList() {
    entries.clear();

   #if MS_HAVE_BINARYDATA
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i) {
        const char* nm = BinaryData::namedResourceList[i];
        if (! isPresetResource (nm)) continue;
        auto e = new PresetEntry(); e->factory = true; e->resName = nm;
        auto base = File (String (nm)).getFileNameWithoutExtension();
        e->name = base.replace ("_", " ");
        entries.add (e);
    }
   #endif

    // User folder
    auto dir = getUserDir(); dir.createDirectory();
    RangedDirectoryIterator it (dir, false, "*.minisynth.json;*.minisynth.xml");
    for (const auto& f : it) {
        auto e = new PresetEntry(); e->factory = false; e->file = f.getFile(); e->name = e->file.getFileNameWithoutExtension();
        entries.add (e);
    }
}

StringArray PresetManager::getAllPresetNames() const {
    StringArray out; for (auto* e : entries) out.add (e->name); return out;
}

bool PresetManager::isFactoryIndex (int index) const {
    if (! isPositiveAndBelow (index, entries.size())) return false; return entries[index]->factory;
}

void PresetManager::applyJson (const String& jsonText) {
    var v = JSON::parse (jsonText);
    if (! v.isObject()) return;
    auto* obj = v.getDynamicObject();
    if (! obj) return;

    var vals = obj->getProperty ("values");
    if (! vals.isObject()) return;
    auto* dyn = vals.getDynamicObject(); if (! dyn) return;

    for (auto& p : dyn->getProperties()) {
        auto id = p.name.toString();
        auto* param = apvts.getParameter (id);
        if (! param) continue;

        if (auto* f = dynamic_cast<AudioParameterFloat*> (param)) {
            float val = (float) p.value;
            float norm = f->range.convertTo0to1 (val);
            f->beginChangeGesture(); f->setValueNotifyingHost (norm); f->endChangeGesture();
        } else if (auto* b = dynamic_cast<AudioParameterBool*> (param)) {
            float val = (float) p.value; b->beginChangeGesture(); b->setValueNotifyingHost (val > 0.5f ? 1.0f : 0.0f); b->endChangeGesture();
        } else if (auto* c = dynamic_cast<AudioParameterChoice*> (param)) {
            int idx = jlimit (0, c->choices.size() - 1, (int) p.value);
            float norm = c->choices.size() > 1 ? (float) idx / (float) (c->choices.size() - 1) : 0.0f;
            c->beginChangeGesture(); c->setValueNotifyingHost (norm); c->endChangeGesture();
        }
    }
}

void PresetManager::applyPresetByIndex (int index) {
    if (! isPositiveAndBelow (index, entries.size())) return;
    auto* e = entries[index];

    if (e->factory) {
       #if MS_HAVE_BINARYDATA
        int size = 0; auto* data = BinaryData::getNamedResource (e->resName.toRawUTF8(), size);
        if (data && size > 0) applyJson (String::fromUTF8 ((const char*) data, size));
       #endif
    } else {
        auto text = e->file.loadFileAsString();
        // Could be JSON or XML
        if (text.trimStart().startsWithChar ('{')) {
            applyJson (text);
        } else {
            if (auto xml = XmlDocument::parse (text)) {
                apvts.replaceState (ValueTree::fromXml (*xml));
            }
        }
    }
}

bool PresetManager::saveUserPreset (const String& name) {
    auto dir = getUserDir(); dir.createDirectory();
    auto f = dir.getChildFile (name + ".minisynth.xml");
    auto st = apvts.copyState();
    if (auto xml = st.createXml()) return xml->writeTo (f);
    return false;
}

bool PresetManager::deleteUserPreset (const String& name) {
    auto dir = getUserDir();
    auto f1 = dir.getChildFile (name + ".minisynth.xml");
    auto f2 = dir.getChildFile (name + ".minisynth.json");
    bool ok = true;
    if (f1.existsAsFile()) ok &= f1.deleteFile();
    if (f2.existsAsFile()) ok &= f2.deleteFile();
    rebuildList();
    return ok;
}

File PresetManager::getUserDir() const {
    auto dir = File::getSpecialLocation (File::userApplicationDataDirectory)
                 .getChildFile (organisation).getChildFile (application).getChildFile ("Presets");
    return dir;
}

} // namespace presets
