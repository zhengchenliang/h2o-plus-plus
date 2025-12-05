![](https://raw.githubusercontent.com/zhengchenliang/h2o-plus-plus/main/_a5472deplmoni0v2.png)

# Campaign Management Tools: campdown and campdeal

This document describes how to use the `campdown` and `campdeal` tools for managing campaign-related JIRA tickets.

## Overview

- **campdown**: Downloads and processes campaign data from `campaigns.json` and JIRA issues, creating serialized data files and text reports.
- **campdeal**: Analyzes JIRA ticket status mismatches based on campaign data and can automatically execute status transitions.

## Prerequisites

### Python Environment

The tools require Python with the `jira` library. Set up a virtual environment:

```bash
cd /root/G1/new2/A5472
python -m venv a5472_venv
source a5472_venv/bin/activate
pip install jira
deactivate
```

### Required Files

- `campaigns.json`: JSON file containing campaign status information
  - Format: `{"CampaignName": {"go": true/false}, ...}`
  - `go: true` = ENABLED, `go: false` = DISABLED

### JIRA Configuration

JIRA connection settings are configured in `_f5472jirafunc0v1.hh`:
- Server: `https://its.cern.ch/jira`
- Project: `PRCAMPAIGNS`
- Authentication: Token-based

## Tool Usage

### campdown

**Purpose**: Download and process campaign and JIRA data.

**Usage**:
```bash
./a5472campdown0v1
```

**What it does**:
1. Reads `campaigns.json` and extracts campaign status (ENABLED/DISABLED)
2. Fetches JIRA issues from the PRCAMPAIGNS project (with pagination)
3. Serializes data to binary files:
   - `cc_json.dat`: Campaign status data
   - `cc_jira.dat`: JIRA issues data
4. Generates text reports:
   - `cc_json.txt`: Full campaign list with status
   - `cc_jira.txt`: Full JIRA issues list

**Output**:
- Console: Summary statistics and first 10 issues
- Files: Binary `.dat` files and text `.txt` reports

**Notes**:
- Fetches issues in batches of 100 to avoid connection issues
- Shows progress: `Fetching batch: startAt=X, maxResults=100... got Y issues (total: Z)`
- Default max: 100,000 issues (configurable in `jira_c` struct)

### campdeal

**Purpose**: Analyze status mismatches and optionally execute transitions.

**Usage**:
```bash
# Analysis only (no execution)
./a5472campdeal0v1

# Execute transitions for specific issue numbers
./a5472campdeal0v1 "1-500,1000,2000-2100"
```

**Range Format**:
- Single numbers: `100`, `200`, `300`
- Ranges: `1-500`, `1000-2000`
- Mixed: `1-5,35-60,101,200-300`
- Examples:
  - `"1-500"` - Issues 1 through 500
  - `"1-5,35-60,101"` - Issues 1-5, 35-60, and 101
  - `"200-300,500"` - Issues 200-300 and 500

**What it does**:
1. Loads campaign status from `cc_json.dat`
2. Loads JIRA issues from `cc_jira.dat`
3. Extracts campaign names from JIRA ticket summaries/descriptions
4. Determines expected status for each ticket based on campaigns
5. Identifies mismatches between current and expected status
6. Generates mismatch reports:
   - `mismatch_alpha.txt`: Issues with JIRA_ONLY campaigns (not in JSON)
   - `mismatch_beta.txt`: Issues without JIRA_ONLY campaigns
   - `mismatch_alpha.dat`: Serialized alpha mismatches
   - `mismatch_beta.dat`: Serialized beta mismatches
7. If issue numbers provided: Executes transitions for matching issues with mismatches

**Output Files**:

**mismatch_alpha.txt** (with JIRA_ONLY campaigns):
```
Mismatch #1 PRCAMPAIGNS-87
  Summary:   New Campaigns: Run3Winter20CosmicGS
  Status:    To Do -> Open
  Action:    Add Campaign
  JIRA_ONLY: Run3Winter20CosmicGS
  Reason:    Has JIRA only + 45d (<60)
```

**mismatch_beta.txt** (without JIRA_ONLY campaigns):
```
Mismatch #1 PRCAMPAIGNS-100
  Summary:   Campaign: Run3Summer2024
  Status:    Open -> Enabled
  Action:    Enable
  JSON:
             Run3Summer2024                        [ENABLED]
  Reason:    All enabled
```

**Execution Mode**:
When issue numbers are provided, the tool will:
- Filter to only issues with status mismatches
- Execute transitions sequentially
- Show progress: `[1/442] Processing PRCAMPAIGNS-1... OK`
- Skip issues that:
  - Don't exist in the data
  - Don't have mismatches (already correct status)

## Status Workflow

### JIRA Ticket Statuses

- **To Do**: Initial state, no campaigns added yet
- **Open**: Campaigns exist but some are disabled
- **Enabled**: All campaigns are enabled
- **Closed**: No campaigns or old JIRA_ONLY campaigns (>=60 days)

### Status Determination Rules

The expected status is determined by:

1. **No campaigns found**: → `Closed`
2. **Has disabled campaigns** (in JSON): → `Open`
3. **Has only enabled campaigns** (in JSON): → `Enabled`
4. **Has JIRA_ONLY campaigns** (not in JSON):
   - If created >= 60 days ago: → `Closed`
   - If created < 60 days ago: → `To Do`

**Note**: Days are calculated from ticket **creation time**, not update time.

### Transition Actions

The tool automatically determines the transition sequence:

| From → To | Actions |
|-----------|---------|
| To Do → Open | `Add Campaign` |
| To Do → Closed | `Closed` |
| To Do → Enabled | `Add Campaign`, `Enable` |
| Open → Closed | `Closed` |
| Open → Enabled | `Enable` |
| Enabled → Open | `Disable` |
| Enabled → Closed | `Closed` |
| Closed → Open | `Reopen` |
| Closed → Enabled | `Reopen`, `Enable` |

## Campaign Extraction

Campaigns are extracted from JIRA ticket summaries and descriptions using:

1. **Text preprocessing**:
   - Removes URLs
   - Expands bracket patterns: `prefix[A|B|C]` → `prefixA prefixB prefixC`
   - Removes `cmsDriver.py` command lines
   - Removes secondary dataset paths (`/Neutrino`, `/MinBias`, etc.)

2. **Campaign matching**:
   - First pass: Exact matches against `campaigns.json`
   - Second pass: Potential new campaigns (JIRA_ONLY) matching campaign name patterns

3. **Veto logic**:
   - JIRA_ONLY campaigns that are prefixes of all JSON campaigns are removed
   - Example: If JSON has `Run3Summer2024A` and `Run3Summer2024B`, JIRA_ONLY `Run3Summer2024` is vetoed

## File Formats

### Input Files

**campaigns.json**:
```json
{
  "Run3Summer2024": {"go": true},
  "Run3Winter2024": {"go": false},
  "Run3Spring2024": {"go": true}
}
```

### Output Files

**Binary (.dat files)**:
- `cc_json.dat`: Serialized campaign status map
- `cc_jira.dat`: Serialized JIRA issues vector
- `mismatch_alpha.dat`: Serialized alpha mismatches
- `mismatch_beta.dat`: Serialized beta mismatches

**Text (.txt files)**:
- `cc_json.txt`: Human-readable campaign list
- `cc_jira.txt`: Human-readable JIRA issues list
- `mismatch_alpha.txt`: Alpha mismatch report
- `mismatch_beta.txt`: Beta mismatch report

## Examples

### Basic Workflow

```bash
# 1. Download latest data
./a5472campdown0v1

# 2. Analyze mismatches
./a5472campdeal0v1

# 3. Review reports
cat mismatch_alpha.txt
cat mismatch_beta.txt

# 4. Execute fixes for specific issues
./a5472campdeal0v1 "1-100"
```

### Execute Specific Issues

```bash
# Fix issues 1-50
./a5472campdeal0v1 "1-50"

# Fix specific issues
./a5472campdeal0v1 "100,200,300"

# Fix multiple ranges
./a5472campdeal0v1 "1-100,200-300,500"
```

### Logging Output

```bash
# Save output to log file
./a5472campdeal0v1 "1-500" | tee -a campdeal_1.log
```

## Troubleshooting

### Connection Issues

**Problem**: `ChunkedEncodingError` or `IncompleteRead` when fetching JIRA issues

**Solution**: 
- The tool uses pagination (100 issues per batch) to avoid this
- If issues persist, reduce `max_results` in `jira_c` config
- Check network connectivity to JIRA server

### Execution Hangs

**Problem**: Execution appears stuck

**Solution**:
- The tool now shows progress: `[X/Y] Processing PRCAMPAIGNS-XXX...`
- Check which issue it's stuck on
- May be network timeout - check JIRA server status
- Python output may be buffered - progress output shows current issue

### No Campaigns Found

**Problem**: Tickets show "No campaigns" or status is always "Closed"

**Solution**:
- Check that `campaigns.json` exists and is valid JSON
- Verify campaign names in JSON match patterns in JIRA tickets
- Review `cc_json.txt` to see loaded campaigns

### Transition Errors

**Problem**: `Transition 'X' not found for PRCAMPAIGNS-Y`

**Solution**:
- Check available transitions in JIRA for that issue
- Verify the issue workflow allows the transition
- Some transitions may require specific conditions in JIRA

### Old Data Files

**Problem**: Deserialization errors or missing fields

**Solution**:
- Re-run `campdown` to regenerate `.dat` files
- Data format may have changed (e.g., added `created` field)
- Delete old `.dat` files and regenerate

## Configuration

### JIRA Settings

Edit `_f5472jirafunc0v1.hh` to modify:

```cpp
struct jira_c
{
  std::string server = "https://its.cern.ch/jira";
  std::string token = "YOUR_TOKEN";
  std::string query = "project=PRCAMPAIGNS ORDER BY updated DESC, priority DESC";
  int timeout = 45;
  int max_results = 100000;
};
```

### Batch Size

Pagination batch size is set in `jira_fetch_campaign_()`:
```python
batch_size = 100  # Adjust if needed
```

## Notes

- **Days calculation**: Uses ticket **creation time**, not update time
- **Campaign matching**: Case-sensitive exact matching against JSON
- **JIRA_ONLY campaigns**: Must match campaign name pattern (alphanumeric, underscores, hyphens)
- **Execution**: Only executes transitions for issues with mismatches
- **Progress**: Execution shows `[X/Y] Processing...` for visibility

## See Also

- JIRA API documentation: https://developer.atlassian.com/cloud/jira/platform/rest/v3/
- Python JIRA library: https://jira.readthedocs.io/

