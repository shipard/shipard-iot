#!/usr/bin/env php
<?php


function parseArgs($argv)
{
	array_shift ($argv);
	$out = array();
	foreach ($argv as $arg){
		if (substr($arg,0,2) == '--'){
			$eqPos = strpos($arg,'=');
			if ($eqPos === false){
				$key = substr($arg,2);
				$out[$key] = isset($out[$key]) ? $out[$key] : true;
			} else {
				$key = substr($arg,2,$eqPos-2);
				$out[$key] = substr($arg,$eqPos+1);
			}
		} else if (substr($arg,0,1) == '-'){
			if (substr($arg,2,1) == '='){
				$key = substr($arg,1,1);
				$out[$key] = substr($arg,3);
			} else {
				$chars = str_split(substr($arg,1));
				foreach ($chars as $char){
					$key = $char;
					$out[$key] = isset($out[$key]) ? $out[$key] : true;
				}
			}
		} else {
			$out[] = $arg;
		}
	}
	return $out;
}

/**
 * Class BuildApp
 */
class BuildApp
{
	var $arguments = NULL;
	var $projectsCfg = NULL;
	var $localCfg = NULL;
	var $libCfg = NULL;

	var $buildChannel = 'stable';
	var $buildCommit = '';
	var $buildVersionId = '';
	var $fwFolderRoot = '';
	var $fwFolder = '';

	var $fwProjects =[];

	var $anyCfgError = TRUE;

	public function __construct($argv)
	{
		if ($argv)
			$this->arguments = parseArgs($argv);

		$this->projectsCfg = $this->loadCfgFile('projects.json');
		if (!$this->projectsCfg)
		{
			$this->err("File projects.json not found or has syntax error!");
			return;
		}

		$this->localCfg = $this->loadCfgFile('local-config.json');
		if (!$this->localCfg)
		{
			$this->err("File local-config.json not found or has syntax error!");
			return;
		}

		$this->libCfg = $this->loadCfgFile('../libs/version.json');
		if (!$this->libCfg)
		{
			$this->err("File ../libs/version.json not found or has syntax error!");
			return;
		}

		$this->buildChannel = 'devel';
		$this->buildCommit = shell_exec("git log --pretty=format:'%h' -n 1");

		$this->buildVersionId = $this->libCfg['version'].'-'.$this->buildCommit;
		if (1)
			$this->buildVersionId .= '-'.base_convert((time() - 1600000000), 10, 36);

		file_put_contents('../libs/versionId.h', "#define SHP_LIBS_VERSION \"{$this->buildVersionId}\"\n");

		if (!is_dir('logs'))
			mkdir('logs');

		$this->fwFolderRoot = 'fw/'.$this->buildChannel.'/ib/';
		$this->fwFolder = $this->fwFolderRoot;

		$this->anyCfgError = FALSE;
	}

	public function arg ($name, $defaultValue = FALSE)
	{
		if (isset ($this->arguments [$name]))
			return $this->arguments [$name];

		return $defaultValue;
	}

	public function loadCfgFile ($fileName)
	{
		if (is_file ($fileName))
		{
			$cfgString = file_get_contents ($fileName);
			if (!$cfgString)
				return FALSE;
			$cfg = json_decode ($cfgString, true);
			if (!$cfg)
				return FALSE;
			return $cfg;
		}
		return FALSE;
	}

	public function command ($idx = 0)
	{
		if (isset ($this->arguments [$idx]))
			return $this->arguments [$idx];

		return "";
	}

	public function err ($msg)
	{
		if ($msg === FALSE)
			return TRUE;

		if (is_array($msg))
		{
			if (count($msg) !== 0)
			{
				forEach ($msg as $m)
					echo ("! " . $m['text']."\n");
				return FALSE;
			}
			return TRUE;
		}

		echo ("ERROR: ".$msg."\n");
		return FALSE;
	}

	function addBuildProjectFWFile ($projectId, $variantId, &$fwFiles)
	{
		$srcFileName = '.pio/build/'.$variantId.'/firmware.bin';
		if (!is_file($srcFileName))
		{
			echo "\n !!! ERROR: firmware file `{$srcFileName}` not found !!!\n";
			return FALSE;
		}

		$versionId = $this->buildVersionId;
		$dstPathCore = '../BUILD/'.$this->fwFolder.'/'.$projectId;
		$dstPath = $dstPathCore.'/'.$versionId;

		$dstBaseFileName = /*$projectId.'-'. */$variantId.'-'.$this->buildVersionId.'-fw.bin';
		$dstFileName = $dstPath.'/'.$dstBaseFileName;

		echo $dstBaseFileName.'; ';

		copy($srcFileName, $dstFileName);

		$fileSize = filesize($dstFileName);
		echo $fileSize.'B; ';

		$checkSum = sha1_file($dstFileName);
		echo $checkSum;

		$fwFiles['files'][] = ['fwId' => $projectId.'-'.$variantId, 'fileName' => $dstBaseFileName, 'size' => $fileSize, 'sha1' => $checkSum];

		return TRUE;
	}

	function buildProject($projectId)
	{
		echo ("### ".$projectId." ###\n");

		chdir("../".$projectId);
		$versionId = $this->buildVersionId;

		$dstPathCore = '../BUILD/'.$this->fwFolder.'/'.$projectId;
		$dstPath = $dstPathCore.'/'.$versionId;

		$now = new \DateTime();
		$fwFiles = [
			'version' => $versionId,
			'timestamp' => $now->format('Y-m-d H:i:s'),
			'files' => []
		];

		if (!is_dir($dstPath))
			mkdir ($dstPath, 0700, TRUE);

		foreach ($this->projectsCfg['projects'][$projectId]['variants'] as $variantId => $variantCfg)
		{
			$logFileName = '../BUILD/logs/'.$projectId.'-'.$variantId.'.log';

			$vt = sprintf("%-22s: ", $variantId);
			echo (" -> ".$vt);

			$cmd = $this->localCfg['pioCmd'].' run -s -e '.$variantId.' 2> '.$logFileName;
			$buildResultCode = 0;
			passthru($cmd, $buildResultCode);

			if ($buildResultCode != 0)
			{
				echo " !!! BUILD ERROR !!!\n";
				return FALSE;
			}

			if (!$this->addBuildProjectFWFile($projectId, $variantId, $fwFiles))
			{
				return FALSE;
			}

			echo ("\n");
		}

		chdir('../BUILD');

		$now = new \DateTime();
		$versionInfo = [
			'version' => $this->buildVersionId,
			'timestamp' => $now->format('Y-m-d H:i:s'),
		];

		file_put_contents($dstPath.'/files.json', json_encode($fwFiles, JSON_PRETTY_PRINT|JSON_UNESCAPED_UNICODE|JSON_UNESCAPED_SLASHES));

		file_put_contents($dstPathCore.'/version.json', json_encode($versionInfo, JSON_PRETTY_PRINT|JSON_UNESCAPED_UNICODE|JSON_UNESCAPED_SLASHES));
		file_put_contents($dstPathCore.'/VERSION',$this->buildVersionId);

		$this->fwProjects[$projectId] = [
			'version' => $versionId,
			'timestamp' => $now->format('Y-m-d H:i:s'),
		];

		return TRUE;
	}

	function buildAll()
	{
		if ($this->anyCfgError)
			return FALSE;

		array_map ('unlink', glob ('logs/*'));
		exec ('rm -rf '.$this->fwFolderRoot);
		if (!is_dir($this->fwFolder))
			mkdir($this->fwFolder, 0700, TRUE);

		foreach ($this->projectsCfg['projects'] as $projectId => $projectCfg)
		{
			if (!$this->buildProject($projectId))
				return FALSE;
		}

		file_put_contents($this->fwFolder.'/projects.json', json_encode($this->fwProjects, JSON_PRETTY_PRINT|JSON_UNESCAPED_UNICODE|JSON_UNESCAPED_SLASHES));

		$doUpload = $this->arg('upload');
		if ($doUpload === TRUE)
			$this->upload();
		$doUploadLocal = $this->arg('upload-local');
		if ($doUploadLocal === TRUE)
			$this->upload(TRUE);

		return TRUE;
	}

	function upload($local = FALSE)
	{
		if ($local)
		{
			echo "--- UPLOAD LOCAL ---\n";
			$remoteUser = $this->localCfg['remoteUserLocal'];
			$remoteServer = $this->localCfg['remoteServerLocal'];
			$remoteDir = '/var/www/iot-boxes/fw/ib/local';//.$this->buildChannel;
		}
		else
		{
			echo "--- UPLOAD ---\n";
			$remoteUser = $this->localCfg['remoteUser'];
			$remoteServer = $this->localCfg['remoteServer'];
			$remoteDir = '/var/www/shpd-webs/download.shipard.org/shipard-iot/fw/ib/'.$this->buildChannel;
		}
		foreach ($this->projectsCfg['projects'] as $projectId => $projectCfg)
		{
			$versionId = $this->buildVersionId;
			$uploadCmd = "ssh -l {$remoteUser} {$remoteServer} mkdir -p $remoteDir/$projectId".'/'. $versionId;
			echo $uploadCmd."\n";
			passthru($uploadCmd);

			$uploadCmd = "scp ".$this->fwFolder.'/'.$projectId.'/'.$versionId . "/* {$remoteUser}@{$remoteServer}:/$remoteDir/".$projectId.'/'.$versionId;
			echo $uploadCmd."\n";
			passthru($uploadCmd);

			$uploadCmd = "scp ".$this->fwFolder.'/'.$projectId.'/'."/*.json {$remoteUser}@{$remoteServer}:/$remoteDir/".$projectId;
			echo $uploadCmd."\n";
			passthru($uploadCmd);

			$uploadCmd = "scp ".$this->fwFolder."/*.json {$remoteUser}@{$remoteServer}:/$remoteDir/";
			echo $uploadCmd."\n";
			passthru($uploadCmd);

			echo "--- DONE ---\n";
		}
	}

	public function run ()
	{
		switch ($this->command ())
		{
			case	'build-all':     				return $this->buildAll();
		}
		echo ("unknown or nothing param....\n");
		echo (" * build-all [--upload | --upload-local]\n");
		return FALSE;
	}
}

$myApp = new BuildApp ($argv);
$myApp->run ();
