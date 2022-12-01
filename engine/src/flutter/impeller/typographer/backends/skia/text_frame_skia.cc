// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/typographer/backends/skia/text_frame_skia.h"

#include <vector>

#include "flutter/fml/logging.h"
#include "impeller/typographer/backends/skia/typeface_skia.h"
#include "include/core/SkFontTypes.h"
#include "include/core/SkRect.h"
#include "third_party/skia/include/core/SkFont.h"
#include "third_party/skia/include/core/SkFontMetrics.h"
#include "third_party/skia/src/core/SkStrikeSpec.h"    // nogncheck
#include "third_party/skia/src/core/SkTextBlobPriv.h"  // nogncheck

namespace impeller {

static Font ToFont(const SkTextBlobRunIterator& run, Scalar scale) {
  auto& font = run.font();
  auto typeface = std::make_shared<TypefaceSkia>(font.refTypefaceOrDefault());

  SkFontMetrics sk_metrics;
  font.getMetrics(&sk_metrics);

  Font::Metrics metrics;
  metrics.scale = scale;
  metrics.point_size = font.getSize();
  metrics.ascent = sk_metrics.fAscent;
  metrics.descent = sk_metrics.fDescent;
  metrics.min_extent = {sk_metrics.fXMin, sk_metrics.fTop};
  metrics.max_extent = {sk_metrics.fXMax, sk_metrics.fBottom};

  std::vector<SkRect> glyph_bounds;
  SkPaint paint;

  glyph_bounds.resize(run.glyphCount());
  run.font().getBounds(run.glyphs(), run.glyphCount(), glyph_bounds.data(),
                       nullptr);
  for (auto& bounds : glyph_bounds) {
    metrics.min_extent = metrics.min_extent.Min({bounds.fLeft, bounds.fTop});
    metrics.max_extent =
        metrics.max_extent.Max({bounds.fRight, bounds.fBottom});
  }

  return Font{std::move(typeface), metrics};
}

TextFrame TextFrameFromTextBlob(const sk_sp<SkTextBlob>& blob, Scalar scale) {
  if (!blob) {
    return {};
  }

  TextFrame frame;

  for (SkTextBlobRunIterator run(blob.get()); !run.done(); run.next()) {
    TextRun text_run(ToFont(run, scale));

    // TODO(jonahwilliams): ask Skia for a public API to look this up.
    // https://github.com/flutter/flutter/issues/112005
    SkStrikeSpec strikeSpec = SkStrikeSpec::MakeWithNoDevice(run.font());
    SkBulkGlyphMetricsAndPaths paths{strikeSpec};

    const auto glyph_count = run.glyphCount();
    const auto* glyphs = run.glyphs();
    switch (run.positioning()) {
      case SkTextBlobRunIterator::kDefault_Positioning:
        FML_DLOG(ERROR) << "Unimplemented.";
        break;
      case SkTextBlobRunIterator::kHorizontal_Positioning:
        FML_DLOG(ERROR) << "Unimplemented.";
        break;
      case SkTextBlobRunIterator::kFull_Positioning:
        for (auto i = 0u; i < glyph_count; i++) {
          // kFull_Positioning has two scalars per glyph.
          const SkPoint* glyph_points = run.points();
          const auto* point = glyph_points + i;
          Glyph::Type type = paths.glyph(glyphs[i])->isColor()
                                 ? Glyph::Type::kBitmap
                                 : Glyph::Type::kPath;

          text_run.AddGlyph(Glyph{glyphs[i], type},
                            Point{point->x(), point->y()});
        }
        break;
      case SkTextBlobRunIterator::kRSXform_Positioning:
        FML_DLOG(ERROR) << "Unimplemented.";
        break;
      default:
        FML_DLOG(ERROR) << "Unimplemented.";
        continue;
    }
    frame.AddTextRun(text_run);
  }

  return frame;
}

}  // namespace impeller
