#include "SAB.hpp"

auto SABBossTracker::CategorizeBoss(const cbtevent& ev, const ag& dst) -> std::optional<SABBossInfo>
{
    switch (dst.prof)
    {
        case 0x2eb4: // storm wizard phase 1
            _hasHitStormWizard = true;
            return std::nullopt;

        case 0xffff32c8: // w1z1
        case 0xffff69f6: // w1z1
        case 0xfffff7f3: // w2z1
        case 0xffff0e2b: // w2z2
            _hasHitStormWizard = false;
            return SABBossInfo{ 1500, dst.prof, SABBossType::Cage };

        case 0xffff0000:            // toad / wizard
            if (_hasHitStormWizard) // storm dragon
                return SABBossInfo{ 1500, dst.prof, SABBossType::Wizard };

            _hasHitStormWizard = false;

            if (ev.dst_instid == 956 || ev.dst_instid == 955)  // frog main hitbox || frog head (moto finger target)
                return SABBossInfo { 600, dst.prof, SABBossType::Toad };

        default:
            return std::nullopt;
    }
}

void SABBossTracker::Reset()
{
    _hasHitStormWizard = false;
}
