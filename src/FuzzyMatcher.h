#ifndef FUZZYMATCHER_H
#define FUZZYMATCHER_H

#include <QString>

// ═══════════════════════════════════════════════════════
// FuzzyMatcher — simple substring match for app launcher
//
// For MVP: case-insensitive substring (reliable, expected).
// score() provides ranking when multiple results match.
// ═══════════════════════════════════════════════════════

class FuzzyMatcher
{
public:
    // Returns true if query is a substring of target (case-insensitive)
    static bool matches(const QString &query, const QString &target)
    {
        if (query.isEmpty())
            return true;
        return target.contains(query, Qt::CaseInsensitive);
    }

    // Score for sorting — higher = better match
    // Exact prefix match scores highest, then word-boundary, then contains
    static int score(const QString &query, const QString &target)
    {
        if (query.isEmpty())
            return 0;
        if (target.isEmpty())
            return -1;

        QString ql = query.toLower();
        QString tl = target.toLower();

        if (!tl.contains(ql))
            return -1;

        int sc = 10;

        // exact prefix → best
        if (tl.startsWith(ql))
            sc += 100;

        // word-boundary match (e.g. "dol" matches start of "Dolphin")
        int idx = tl.indexOf(ql);
        if (idx == 0 || target[idx - 1] == ' ' || target[idx - 1] == '-'
            || target[idx - 1] == '.' || target[idx - 1] == '_')
            sc += 50;

        // shorter target = more relevant
        sc += qMax(0, 30 - target.length());

        return sc;
    }

    static int bestScore(const QString &query, const QStringList &fields)
    {
        int best = -1;
        for (const auto &f : fields) {
            int s = score(query, f);
            if (s > best)
                best = s;
        }
        return best;
    }
};

#endif // FUZZYMATCHER_H
