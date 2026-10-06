#!/usr/bin/env python3
"""固定统计入口：读C隔离原始测量，保留失败分母，按样例最坏值分层统计。
实验recipe未批准时仅写候选/结构失败，不把成功样本预算当正式验收。
"""
import argparse
import csv
import json
import math
import statistics
from pathlib import Path


def quantile(values, p):
    """nearest-rank分位数，不插值，也不删最大/失败样例。"""
    return sorted(values)[math.ceil(p * len(values)) - 1]


def distribution(values, lower=False, step=0.5):
    """唯一预算统计公式；各工具只给单位/上下尾/取整步长。"""
    mean, std = statistics.mean(values), statistics.stdev(values)
    percentile = quantile(values, 0.01 if lower else 0.99)
    raw = min(mean - 3 * std, percentile) if lower else max(mean + 3 * std, percentile)
    rounded = step * (math.floor(raw / step) if lower else math.ceil(raw / step))
    return dict(n=len(values), mean=mean, sample_std=std, percentile=percentile,
                maximum=max(values), minimum=min(values), raw=raw, rounded=rounded, step=step)


def metrics(corners):
    """先求每样例四角/八边最坏值，避免把相关边当独立样本。"""
    return {
        "max_corner_error": max(c["corner_error_px"] for c in corners),
        "max_edge_direction_diff": max(v for c in corners for v in c["direction_diff_deg"]),
        "max_support_extension": max(v for c in corners for v in c["support_extension_px"]),
        "max_line_fit_residual": max(v for c in corners for v in c["line_mean_residual_px"]),
        "min_edge_points": min(v for c in corners for v in c["support_points"]),
        "truth_error_budget": max(c["truth_error_px"] for c in corners),
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("measurements", type=Path)
    parser.add_argument("report", type=Path)
    args = parser.parse_args()
    records = [json.loads(line) for line in args.measurements.read_text().splitlines()]
    if not records or any(r["profile"] != "C" for r in records):
        raise ValueError("只接受非空C测量，禁止以H或视频定预算")
    failures = [r for r in records if not r["success"]]
    strata = {}
    for r in records:
        for kind in ("all", "L", "M"):
            key = (r["work_size"][0], r["radius_original_px"], bool(r["seed"]), kind)
            group = strata.setdefault(key, {"total": 0, "success": []})
            group["total"] += 1
            if r["success"]:
                selected = [c for c in r["corners"] if kind == "all" or c["kind"] == kind]
                group["success"].append((r["measurement_case_id"], metrics(selected)))
    rows = []
    for key, group in sorted(strata.items()):
        if len(group["success"]) < 2:
            continue
        for metric in group["success"][0][1]:
            values = [m[metric] for _, m in group["success"]]
            lower = metric == "min_edge_points"
            step = 1 if lower or metric == "max_edge_direction_diff" else 0.5
            stats = distribution(values, lower, step)
            if lower:
                stats["rounded"] = max(3, stats["rounded"])
            worst = min(group["success"], key=lambda item: item[1][metric]) if lower else max(group["success"], key=lambda item: item[1][metric])
            rows.append(dict(work_width=key[0], radius=key[1], noisy=int(key[2]), kind=key[3], metric=metric,
                             total=group["total"], failures=group["total"]-len(values), **stats, worst_case=worst[0]))
    args.report.parent.mkdir(parents=True, exist_ok=True)
    csv_path = args.report.with_suffix(".csv")
    with csv_path.open("w") as out:
        writer = csv.DictWriter(out, fieldnames=list(rows[0]) if rows else ["status"])
        writer.writeheader()
        writer.writerows(rows)
    candidates = {}
    for row in rows:
        metric, value = row["metric"], row["rounded"]
        if metric not in candidates:
            candidates[metric] = value
        else:
            candidates[metric] = min(candidates[metric], value) if metric == "min_edge_points" else max(candidates[metric], value)
    text = ["# Block3 C实验统计（recipe v1；尚非批准预算）", "",
            f"总样例 {len(records)}，有效 {len(records)-len(failures)}，结构失败 {len(failures)}；预期正式C为720。",
            "", "固定recipe含未批准的简化/位置/圆角剔除/连接弧/跨度/病态参数；只报告实验事实，不能写回生产配置。",
            "任何结构失败未解决前，成功样本的统计候选均不可批准。样本不全也不能称正式C完成。",
            "", "方法：样例内最坏值；样本标准差n−1；nearest-rank P99/P01；按工作尺度/圆角/噪声/L-M分层。",
            "上限max(μ+3s,P99)，像素向上0.5px、角度向上1°；点数min(μ−3s,P01)，向下整数且≥3。",
            "", "| 字段 | 实验候选（非批准值） |", "|---|---:|"]
    text += [f"| {name} | {value:g} |" for name, value in candidates.items()]
    text += ["", f"逐层n/失败数/μ/s/分位数/最大值/最坏case见 `{csv_path.name}`。",
             f"原始证据：`{args.measurements}`。", "", "失败明细："]
    text += [f"- {r['measurement_case_id']}: {r['reason']}" for r in failures] or ["- 无（仅说明本recipe及已测样例）。"]
    text += ["", "用户批准记录：待审。H/全视频/V/N/Q：NOT_RUN/未确认。"]
    args.report.write_text("\n".join(text) + "\n")
    print(json.dumps({"total": len(records), "failures": len(failures), "exploratory_candidates": candidates,
                      "report": str(args.report)}, ensure_ascii=False))


if __name__ == "__main__":
    main()
