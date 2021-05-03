classdef iqread
    properties (Constant)
        constants = struct(...
            'RKName', 128, ...
            'RKFileHeader', 4096, ...
            'RKMaxMatchedFilterCount', 8, ...
            'RKFilterAnchorSize', 64, ...
            'RKMaximumStringLength', 4096, ...
            'RKMaximumPathLength', 1024, ...
            'RKMaximumPrefixLength', 8, ...
            'RKMaximumFolderPathLength', 768, ...
            'RKMaximumWaveformCount', 22, ...
            'RKMaximumFilterCount', 8, ...
            'RKRadarDescOffset', 256, ...
            'RKRadarDesc', 1072, ...
            'RKConfigV1', 1441, ...
            'RKConfig', 1457, ...
            'RKMaximumCommandLength', 512, ...
            'RKMaxFilterCount', 8, ...
            'RKPulseHeader', 256, ...
            'RKWaveFileGlobalHeader', 512);
    end
    properties
        filename = '';
        header = struct('preface', [], 'buildNo', 6, 'desc', [], 'config', [], 'waveform', []);
        pulses = []
    end
    methods
        % Constructor
        function self = iqread(filename, maxPulse)
            if ~exist('maxPulse', 'var'), maxPulse = inf; end
            fprintf('Filename: %s\n', filename);
            self.filename = filename;
            fid = fopen(self.filename);
            if (fid < 0)
                error('Unable to open file.');
            end

            % The very first two: (RKName preface) and (uint32_t buildNo)
            self.header.preface = deblank(fread(fid, [1 self.constants.RKName], 'char=>char'));
            self.header.buildNo = fread(fid, 1, 'uint32');
            
            % Third component: (RKRawDataType dataType)
            if self.header.buildNo >= 5
                self.header.dataType = fread(fid, 1, 'uint8');
                fseek(fid, 123, 'cof');
            end
            
            % Radar description: (RKRadarDesc desc)
            if self.header.buildNo >= 2
                if self.header.buildNo >= 6
                    % RKRadarDescOffset
                    offset = self.constants.RKRadarDescOffset;
                else
                    % RKName * (char) + (uint32_t)
                    offset = self.constants.RKName + 4;
                end
                h = memmapfile(self.filename, ...
                    'Offset', offset, ...
                    'Repeat', 1, ...
                    'Format', { ...
                        'uint32', [1 1], 'initFlags'; ...
                        'uint32', [1 1], 'pulseCapacity'; ...
                        'uint16', [1 1], 'pulseToRayRatio'; ...
                        'uint16', [1 1], 'doNotUse'; ...
                        'uint32', [1 1], 'healthNodeCount'; ...
                        'uint32', [1 1], 'healthBufferDepth'; ...
                        'uint32', [1 1], 'statusBufferDepth'; ...
                        'uint32', [1 1], 'configBufferDepth'; ...
                        'uint32', [1 1], 'positionBufferDepth'; ...
                        'uint32', [1 1], 'pulseBufferDepth'; ...
                        'uint32', [1 1], 'rayBufferDepth'; ...
                        'uint32', [1 1], 'productBufferDepth'; ...
                        'uint32', [1 1], 'controlCapacity'; ...
                        'uint32', [1 1], 'waveformCalibrationCapacity'; ...
                        'uint64', [1 1], 'healthNodeBufferSize'; ...
                        'uint64', [1 1], 'healthBufferSize'; ...
                        'uint64', [1 1], 'statusBufferSize'; ...
                        'uint64', [1 1], 'configBufferSize'; ...
                        'uint64', [1 1], 'positionBufferSize'; ...
                        'uint64', [1 1], 'pulseBufferSize'; ...
                        'uint64', [1 1], 'rayBufferSize'; ...
                        'uint64', [1 1], 'productBufferSize'; ...
                        'uint32', [1 1], 'pulseSmoothFactor'; ...
                        'uint32', [1 1], 'pulseTicsPerSecond'; ...
                        'uint32', [1 1], 'positionSmoothFactor'; ...
                        'uint32', [1 1], 'positionTicsPerSecond'; ...
                        'double', [1 1], 'positionLatency'; ...
                        'double', [1 1], 'latitude'; ...
                        'double', [1 1], 'longitude'; ...
                        'single', [1 1], 'heading'; ...
                        'single', [1 1], 'radarHeight'; ...
                        'single', [1 1], 'wavelength'; ...
                        'uint8',  [1 self.constants.RKName], 'name_raw'; ...
                        'uint8',  [1 self.constants.RKMaximumPrefixLength], 'filePrefix_raw'; ...
                        'uint8',  [1 self.constants.RKMaximumFolderPathLength], 'dataPath_raw'});
            elseif self.header.buildNo == 1
                h = memmapfile(self.filename, ...
                    'Offset', self.constants.RKName + 4, ...    % RKName * (char) + (uint32_t)
                    'Repeat', 1, ...
                    'Format', { ...
                        'uint32', [1 1], 'initFlags'; ...
                        'uint32', [1 1], 'pulseCapacity'; ...
                        'uint32', [1 1], 'pulseToRayRatio'; ...
                        'uint32', [1 1], 'healthNodeCount'; ...
                        'uint32', [1 1], 'configBufferDepth'; ...
                        'uint32', [1 1], 'positionBufferDepth'; ...
                        'uint32', [1 1], 'pulseBufferDepth'; ...
                        'uint32', [1 1], 'rayBufferDepth'; ...
                        'uint32', [1 1], 'controlCount'; ...
                        'double', [1 1], 'latitude'; ...
                        'double', [1 1], 'longitude'; ...
                        'single', [1 1], 'heading'; ...
                        'single', [1 1], 'radarHeight'; ...
                        'single', [1 1], 'wavelength'; ...
                        'uint8',  [1 self.constants.RKName], 'name_raw'; ...
                        'uint8',  [1 self.constants.RKName], 'filePrefix_raw'; ...
                        'uint8',  [1 self.constants.RKMaximumPathLength], 'dataPath_raw'});

            end
            self.header.desc = h.data;
            self.header.desc.name = deblank(char(self.header.desc.name_raw));
            self.header.desc.filePrefix = deblank(char(self.header.desc.filePrefix_raw));
            self.header.desc.dataPath = deblank(char(self.header.desc.dataPath_raw));
            
            % Header->config
            if self.header.buildNo >= 5
                if self.header.buildNo == 6
                    % (RKRadarDescOffset = 256) + (RKRadarDesc) --> RKConfig
                    offset = self.constants.RKRadarDescOffset + self.constants.RKRadarDesc;
                else
                    % (RKName) + (uint32_t) + (RKRadarDesc) --> RKConfigV1
                    offset = self.constants.RKName + 4 + self.constants.RKRadarDesc;
                end
                c1 = memmapfile(self.filename, ...
                    'Offset', offset, ...
                    'Repeat', 1, ...
                    'Format', { ...
                        'uint64', [1 1], 'i'; ...
                        'single', [1 1], 'sweepElevation'; ...
                        'single', [1 1], 'sweepAzimuth'; ...
                        'uint32', [1 1], 'startMarker'; ...
                        'uint8',  [1 1], 'filterCount'});
                % + above
                offset = offset + 21;
                c2 = memmapfile(self.filename, ...
                    'Offset', offset, ...
                    'Repeat', self.constants.RKMaxFilterCount, ...
                    'Format', { ...
                        'uint32', [1 1], 'name'; ...
                        'uint32', [1 1], 'origin'; ...
                        'uint32', [1 1], 'length'; ...
                        'uint32', [1 1], 'inputOrigin'; ...
                        'uint32', [1 1], 'outputOrigin'; ...
                        'uint32', [1 1], 'maxDataLength'; ...
                        'single', [1 1], 'subCarrierFrequency'; ...
                        'single', [1 1], 'sensitivityGain'; ...
                        'single', [1 1], 'filterGain'; ...
                        'single', [1 1], 'fullScale'; ...
                        'single', [1 1], 'lowerBoundFrequency'; ...
                        'single', [1 1], 'upperBoundFrequency'; ...
                        'uint8',  [1 16], 'padding'});
                % + RKMaxFilterCount * RKFilterAnchorSize
                offset = offset + self.constants.RKMaxFilterCount * self.constants.RKFilterAnchorSize;
                c3 = memmapfile(self.filename, ...
                     'Offset', offset, ...
                     'Repeat', 1, ...
                     'Format', { ...
                        'single', [1 self.constants.RKMaxFilterCount], 'prt'; ...
                        'single', [1 self.constants.RKMaxFilterCount], 'pw'; ...
                        'uint32', [1 1], 'pulseGateCount'; ...
                        'single', [1 1], 'pulseGateSize'; ...
                        'uint32', [1 1], 'pulseRingFilterGateCount'; ...
                        'uint32', [1 self.constants.RKMaxFilterCount], 'waveformId'; ...
                        'single', [1 2], 'noise'; ...
                        'single', [1 2], 'systemZCal'; ...
                        'single', [1 1], 'systemDCal'; ...
                        'single', [1 1], 'systemPCal'; ...
                        'single', [self.constants.RKMaxFilterCount 2], 'ZCal'; ...
                        'single', [self.constants.RKMaxFilterCount 1], 'DCal'; ...
                        'single', [self.constants.RKMaxFilterCount 1], 'PCal'; ...
                        'single', [1 1], 'SNRThreshold'; ...
                        'single', [1 1], 'SQIThreshold'; ...
                        'uint8',  [1 self.constants.RKName], 'waveform_raw'; ...
                        'uint8',  [1 self.constants.RKMaximumCommandLength], 'vcpDefinition_raw'});
                self.header.config = c1.data;
                self.header.config.filterAnchors = c2.data;
                for ii = 1:length(c3.Format) - 2
                    self.header.config.(c3.Format{ii, 3}) = c3.data.(c3.Format{ii, 3});
                end
                self.header.config.waveform = deblank(char(c3.data.waveform_raw));
                self.header.config.vcpDefinition = deblank(char(c3.data.vcpDefinition_raw));
                if self.header.buildNo == 6
                    % Read in RKWaveFileGlobalHeader
                    offset = self.constants.RKFileHeader;
                    w = memmapfile(self.filename, ...
                        'Offset', offset, ...
                        'Repeat', 1, ...
                        'Format', { ...
                            'uint8',  [1 1], 'count'; ...
                            'uint32', [1 1], 'depth'; ...
                            'uint32', [1 1], 'type'; ...
                            'uint8',  [1 128], 'name'; ...
                            'double', [1 1], 'fc'; ...
                            'double', [1 1], 'fs'; ...
                            'uint8',  [1 self.constants.RKMaximumWaveformCount], 'filterCounts'; ...
                        });
                    self.header.waveform = w.data;
                    self.header.waveform.filterCounts = self.header.waveform.filterCounts(1:self.header.waveform.count);
                    self.header.waveform.name = deblank(char(self.header.waveform.name));
                    offset = offset + self.constants.RKWaveFileGlobalHeader;
                    tones = [];
                    for i = 1:self.header.waveform.count
                        gfilt = [];
                        for j = 1:self.header.waveform.filterCounts(i)
                            % Read in RKFilterAnchor
                            w = memmapfile(self.filename, ...
                                'Offset', offset, ...
                                'Repeat', self.header.waveform.filterCounts(i), ...
                                'Format', { ...
                                    'uint32', [1 1], 'name'; ...
                                    'uint32', [1 1], 'origin'; ...
                                    'uint32', [1 1], 'length'; ...
                                    'uint32', [1 1], 'inputOrigin'; ...
                                    'uint32', [1 1], 'outputOrigin'; ...
                                    'uint32', [1 1], 'maxDataLength'; ...
                                    'single', [1 1], 'subCarrierFrequency'; ...
                                    'single', [1 1], 'sensitivityGain'; ...
                                    'single', [1 1], 'filterGain'; ...
                                    'single', [1 1], 'fullScale'; ...
                                    'single', [1 1], 'lowerBoundFrequency'; ...
                                    'single', [1 1], 'upperBoundFrequency'; ...
                                    'uint8',  [1 16], 'padding'});
                            % Combine the waveform groups
                            gfilt = cat(1, gfilt, w.data);
                            offset = offset + self.constants.RKFilterAnchorSize;
                        end
                        gfilt = rmfield(gfilt, 'padding');
                        % Read in the samples
                        depth = double(self.header.waveform.depth);
                        w2 = memmapfile(self.filename, ...
                            'Offset', offset, ...
                            'Repeat', 1, ...
                            'Format', { ...
                                'single', [2 depth], 'samples'; ...
                                'int16', [2 depth], 'iSamples'});
                        offset = offset + 2 * depth * (4 + 2);
                        x = w2.data.samples;
                        y = w2.data.iSamples;
                        gsamp = struct('samples', permute(complex(x(1, :), x(2, :)), [2 1]), ...
                                   'iSamples', permute(complex(y(1, :), y(2, :)), [2 1]));
                        tone = cell2struct([struct2cell(gfilt); struct2cell(gsamp)], ...
                                              [fieldnames(gfilt); fieldnames(gsamp)]);
                        tones = cat(1, tones, tone);
                    end
                    self.header.waveform.tones = tones;
                elseif self.header.buildNo == 5
                    % (RKName) + (uint32_t) + (RKRadarDesc) + RKConfigV1 --> RKRawDataType
                    offset = self.constants.RKName + 4 + self.constants.RKRadarDesc + self.constants.RKConfigV1;
                    fseek(fid, offset, 'bof');
                    self.header.dataType = fread(fid, 1, 'uint8');
                end
                % if self.header.dataType == 2
                %     str = 'compressed';
                % else
                %     str = 'raw';
                % end
                % fprintf('dataType = %d (%s)\n', self.header.dataType, str);
            elseif self.header.buildNo >= 2 && self.header.buildNo < 5
                % (RKName) + (uint32_t) + (RKRadarDesc) --> RKConfigV1
                offset = self.constants.RKName + 4 + self.constants.RKRadarDesc;
                c = memmapfile(self.filename, ...
                    'Offset', offset, ...
                    'Repeat', 1, ...
                    'Format', { ...
                        'uint64', [1 1], 'i'; ...
                        'single', [1 1], 'sweepElevation'; ...
                        'single', [1 1], 'sweepAzimuth'; ...
                        'uint32', [1 1], 'startMarker'; ...
                        'uint8',  [1 1], 'filterCount'});
                % + above
                offset = offset + 21;
                c2 = memmapfile(self.filename, ...
                    'Offset', offset, ...
                    'Repeat', self.constants.RKMaxFilterCount, ...
                    'Format', { ...
                        'uint32', [1 1], 'name'; ...
                        'uint32', [1 1], 'origin'; ...
                        'uint32', [1 1], 'length'; ...
                        'uint32', [1 1], 'inputOrigin'; ...
                        'uint32', [1 1], 'outputOrigin'; ...
                        'uint32', [1 1], 'maxDataLength'; ...
                        'single', [1 1], 'subCarrierFrequency'; ...
                        'single', [1 1], 'sensitivityGain'; ...
                        'single', [1 1], 'filterGain'; ...
                        'single', [1 1], 'fullScale'; ...
                        'single', [1 1], 'lowerBoundFrequency'; ...
                        'single', [1 1], 'upperBoundFrequency'; ...
                        'uint8',  [1 16], 'padding'});
                % + RKMaxFilterCount * RKFilterAnchorSize
                offset = offset + self.constants.RKMaxFilterCount * self.constants.RKFilterAnchorSize;
                c3 = memmapfile(self.filename, ...
                     'Offset', offset, ...
                     'Repeat', 1, ...
                     'Format', { ...
                        'uint32', [1 self.constants.RKMaxFilterCount], 'pw'; ...
                        'uint32', [1 self.constants.RKMaxFilterCount], 'prf'; ...
                        'uint32', [1 1], 'pulseGateCount'; ...
                        'single', [1 1], 'pulseGateSize'; ...
                        'uint32', [1 1], 'pulseRingFilterGateCount'; ...
                        'uint32', [1 self.constants.RKMaxFilterCount], 'waveformId'; ...
                        'single', [1 2], 'noise'; ...
                        'single', [1 2], 'systemZCal'; ...
                        'single', [1 1], 'systemDCal'; ...
                        'single', [1 1], 'systemPCal'; ...
                        'single', [self.constants.RKMaxFilterCount 2], 'ZCal'; ...
                        'single', [self.constants.RKMaxFilterCount 1], 'DCal'; ...
                        'single', [self.constants.RKMaxFilterCount 1], 'PCal'; ...
                        'single', [1 1], 'SNRThreshold'; ...
                        'uint8',  [1 self.constants.RKName], 'waveform_raw'; ...
                        'uint8',  [1 self.constants.RKMaximumCommandLength], 'vcpDefinition_raw'});
                self.header.config = c.data;
                self.header.config.filterAnchors = c2.data;
                % Remainder of RKConfig
                for ii = 1:length(c3.Format) - 2
                    self.header.config.(c3.Format{ii, 3}) = c3.data.(c3.Format{ii, 3});
                end
                self.header.config.waveformName = deblank(char(c3.data.waveform_raw));
                self.header.config.vcpDefinition = deblank(char(c3.data.vcpDefinition_raw));
                self.header.dataType = 1;
            elseif self.header.buildNo == 1
                c = memmapfile(self.filename, ...
                    'Offset', self.constants.RKName + 4 + self.constants.RKRadarDesc, ... % RKName * (char) + (uint32_t) + (RKRadarDesc)
                    'Repeat', 1, ...
                    'Format', { ...
                        'uint64', [1 1], 'i'; ...
                        'uint32', [1 self.constants.RKMaxFilterCount], 'pw'; ...
                        'uint32', [1 self.constants.RKMaxFilterCount], 'prf'; ...
                        'uint32', [1 self.constants.RKMaxFilterCount], 'gateCount'; ...
                        'uint32', [1 self.constants.RKMaxFilterCount], 'waveformId'; ...
                        'single', [1 2], 'noise'; ...
                        'single', [self.constants.RKMaxFilterCount 2], 'ZCal'; ...
                        'single', [self.constants.RKMaxFilterCount 1], 'DCal'; ...
                        'single', [self.constants.RKMaxFilterCount 1], 'PCal'; ...
                        'single', [1 1], 'SNRThreshold'; ...
                        'single', [1 1], 'sweepElevation'; ...
                        'single', [1 1], 'sweepAzimuth'; ...
                        'uint32', [1 1], 'startMarker'; ...
                        'uint8',  [1 self.constants.RKName], 'waveform_name_raw'; ...
                        'uint8',  [1 self.constants.RKName], 'vcpDefinition_raw'});
                self.header.config = c.data;
                self.header.config.waveformName = deblank(char(self.header.config.waveform_name_raw));
                self.header.config.vcpDefinition = deblank(char(self.header.config.vcpDefinition_raw));
                self.header.dataType = 1;
            end

            % Partially read the very first pulse
            if self.header.buildNo <= 5
                fseek(fid, self.constants.RKFileHeader + 28, 'bof');
            else
                fseek(fid, offset + 28, 'bof');
            end
            capacity = fread(fid, 1, 'uint32');
            gateCount = fread(fid, 1, 'uint32');
            downSampledGateCount = fread(fid, 1, 'uint32');
            fclose(fid);
            fprintf('gateCount = %d   capacity = %d   downSampledGateCount = %d\n', gateCount, capacity, downSampledGateCount);
            
            % Some dimensions
            if isfinite(maxPulse)
                fprintf('Reading %d pulses ...\n', maxPulse);
            else
                fprintf('Reading pulses ...\n');
            end

            % Pulses
            if self.header.buildNo <= 5
                offset = self.constants.RKFileHeader;
            end
            if self.header.dataType == 2
                fprintf('offset = %d\n', offset);
                % Compressed I/Q
                m = memmapfile(self.filename, ...
                    'Offset', offset, ...
                    'Repeat', maxPulse, ...
                    'Format', { ...
                        'uint64', [1 1], 'i'; ...
                        'uint64', [1 1], 'n'; ...
                        'uint64', [1 1], 't'; ...
                        'uint32', [1 1], 's'; ...
                        'uint32', [1 1], 'capacity'; ...
                        'uint32', [1 1], 'gateCount'; ...
                        'uint32', [1 1], 'downSampledGateCount'; ...
                        'uint32', [1 1], 'marker'; ...
                        'uint32', [1 1], 'pulseWidthSampleCount'; ...
                        'uint64', [1 1], 'time_tv_sec'; ...
                        'uint64', [1 1], 'time_tv_usec'; ...
                        'double', [1 1], 'timeDouble'; ...
                        'uint8',  [1 4], 'rawAzimuth'; ...
                        'uint8',  [1 4], 'rawElevation'; ...
                        'uint16', [1 1], 'configIndex'; ...
                        'uint16', [1 1], 'configSubIndex'; ...
                        'uint16', [1 1], 'azimuthBinIndex'; ...
                        'single', [1 1], 'gateSizeMeters'; ...
                        'single', [1 1], 'elevationDegrees'; ...
                        'single', [1 1], 'azimuthDegrees'; ...
                        'single', [1 1], 'elevationVelocityDegreesPerSecond'; ...
                        'single', [1 1], 'azimuthVelocityDegreesPerSecond'; ...
                        'single', [2 downSampledGateCount 2], 'iq'});
            else
                % Raw I/Q straight from the transceiver
                m = memmapfile(self.filename, ...
                    'Offset', offset, ...
                    'Repeat', maxPulse, ...
                    'Format', { ...
                        'uint64', [1 1], 'i'; ...
                        'uint64', [1 1], 'n'; ...
                        'uint64', [1 1], 't'; ...
                        'uint32', [1 1], 's'; ...
                        'uint32', [1 1], 'capacity'; ...
                        'uint32', [1 1], 'gateCount'; ...
                        'uint32', [1 1], 'downSampledGateCount'; ...
                        'uint32', [1 1], 'marker'; ...
                        'uint32', [1 1], 'pulseWidthSampleCount'; ...
                        'uint64', [1 1], 'time_tv_sec'; ...
                        'uint64', [1 1], 'time_tv_usec'; ...
                        'double', [1 1], 'timeDouble'; ...
                        'uint8',  [1 4], 'rawAzimuth'; ...
                        'uint8',  [1 4], 'rawElevation'; ...
                        'uint16', [1 1], 'configIndex'; ...
                        'uint16', [1 1], 'configSubIndex'; ...
                        'uint16', [1 1], 'azimuthBinIndex'; ...
                        'single', [1 1], 'gateSizeMeters'; ...
                        'single', [1 1], 'elevationDegrees'; ...
                        'single', [1 1], 'azimuthDegrees'; ...
                        'single', [1 1], 'elevationVelocityDegreesPerSecond'; ...
                        'single', [1 1], 'azimuthVelocityDegreesPerSecond'; ...
                        'int16',  [2 gateCount 2], 'iq'});
            end
            self.pulses = m.Data;
        end
    end
end
